/*
	:: C D I 4 D C ::

	By SiZiOUS, http://sbibuilder.shorturl.com/
	5 july 2006, v0.2b

	File : 	MAIN.C
	Desc : 	The main file for cd4dc.

			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.


	Changes :
		0.3b :	13 april 2007
				- Added a new image generation method : Data/Data images.
				- Removed the algo that checks if the ISO is a veritable 11702 because it fails.

		0.2b : 	6 july 2006
				- If the iso doesn't exists, cdi4dc don't create an empty CDI file. (Thx Fackue)
				- Fixed a problem with CD Labels with spaces (Thx Fackue)
				- Checks if the ISO is a veritable 11702 iso.

		0.1b : 	5 july 2006
				Initial release
*/

#include "cdibuild.h"
#include "tools.h"
#include <stdlib.h>

#define BUILD_DATE "14 may 2022"
#define VERSION "0.5c"
#define WARNING_MSG "(TO A CD-RW PLEASE AGAIN... BETA VERSION !!!)"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

uint32_t x, y; // position ou on doit placer le curseur avant d'écrire le pourcentage

int write_cdi_audio_track(FILE *cdi);
void write_audio_cdi_header(FILE *cdi, char* cdiname, char* volume_name, long data_sector_count, uint32_t total_cdi_space_used);
int write_data_cdi_header(FILE *cdi, char* cdiname, char* volume_name, long data_sector_count, uint32_t total_cdi_space_used);
void write_data_gap_start_track(FILE* cdi);
void write_data_header_boot_track(FILE* cdi, FILE* iso);

void print_help() {
	printf("%sThis proggy was written in order to replace the old (but good) Xeal's bin2boot.\nIt generates a %s*REAL*%s valid CDI file from an ISO.\n\n", KWHT, KRED, KWHT);
	printf("%sSyntax: ", KBLU);
	printf("%s%s <input.iso> <output.cdi> [-d]\n\n", KCYN, get_program_name());

	printf("%sOptions:\n", KBLU);
	printf("%s  -d        : Generates a Data/Data image from a MSINFO 0 ISO.\n  <nothing> : Generates a Audio/Data image from a MSINFO 11702 ISO.\n\n", KCYN);

	printf("%sThis proggy's still dedicated to Ron even if today it isn't the 6 july !\n\nGreetings goes to:\n  BlackAura, Fackue, Xeal, DeXT, Heiko Eissfeldt and Joerg Schilling.\n\n", KBLU);
	printf("%sSiZ! for Dreamcast-Scene / %s... The legend will never die...\n", KGRN, BUILD_DATE);
}

void start_progressbar() {
	x =	whereX();
	y = whereY();
}

void padding_event(int sector_count) {
	gotoXY(x, y);
	printf("%s%d block(s) : padding is needed...\n", KRED, sector_count);
}

void writing_data_track_event(uint32_t current_pos, uint32_t total_iso_size) {
	gotoXY(x, y);

	float p1 = (float)current_pos / (float)total_iso_size;
	uint32_t percent = p1 * 100;
	printf("[");
	printf("%s%u/%u", KRED, current_pos, total_iso_size);
	printf("] - ");
	printf("%s%u%%\r", KMAG, percent);
}

void echo(char *label) {
	printf("%s%s", KWHT, label);
}

void print_head() {
	printf("%sCDI4DC", KRED);
	printf("%s - ", KWHT);
	printf("%s%s", KYEL, VERSION);
	printf("%s - Written by SiZiOUS\nhttp://www.sizious.com/\n\n", KWHT);
}

void create_audio_data_image(FILE* infp, FILE* outfp, char* outfilename) {
	char volume_name[33];
    int data_blocks_count;
	float space_used;

	/*i = get_iso_msinfo_value(infp);
	if (i != 11702) { // 11702 pour notre MSINFO !
		textColor(LIGHT_RED);
		printf("Warning: The ISO's LBA seems to be invalid ! Value : %d\n", i);
		textColor(LIGHT_GRAY);
	}*/

	echo("Image method............: ");
	printf("Audio/Data\n");

	get_volumename(infp, volume_name);
	echo("Volume name.............: ");
	printf("%s\n", volume_name);

	// ecrire piste audio
	echo("Writing audio track.....: ");
	write_cdi_audio_track(outfp);
	printf("OK\n");

	// ecrire les tracks GAP
	echo("Writing pregap tracks...: ");
	write_gap_tracks(outfp);
	printf("OK\n");

	//ecrire section data
	echo("Writing datas track.....: ");

	start_progressbar();

	data_blocks_count = write_data_track(outfp, infp);

	space_used = (float)(get_total_cdi_space_used(data_blocks_count) * data_sector_size) / 1024 / 1024;
	gotoXY(x, y);
	printf("%s\n%25c %d ", KCYN, ' ', data_blocks_count);
	printf("block(s) written (");
	printf("%s%5.2fMB ", KGRN, space_used);
	printf("used)\n");

	// ecrire en tête (le footer)
	echo("Writing CDI header......: ");
	write_audio_cdi_header(
		outfp,
		outfilename,
		volume_name,
		data_blocks_count,
		get_total_cdi_space_used(data_blocks_count)
	);
	printf("OK\n");
}

void create_data_data_image(FILE* infp, FILE* outfp, char* outfilename) {
	char volume_name[33];
	int data_blocks_count;
	float space_used;

	echo("Image method............: ");
	printf("Data/Data\n");

	get_volumename(infp, volume_name);
	echo("Volume name.............: ");
	printf("%s\n", volume_name);

	// écrire le début du CDI data/data (GAP 1)
	echo("Writing data pregap.....: ");
	write_data_gap_start_track(outfp);
	printf("OK\n");

	//ecrire section data
	echo("Writing datas track.....: ");

	start_progressbar();

	data_blocks_count = write_data_track(outfp, infp);

	space_used = (float)(get_total_cdi_space_used(data_blocks_count) * data_sector_size) / 1024 / 1024;
	gotoXY(x, y);
	printf("%s\n%25c %d ", KCYN, ' ', data_blocks_count);
	printf("block(s) written (");
	printf("%s%5.2fMB ", KGRN, space_used);
	printf("used)\n");

	// ecrire les tracks GAP
	echo("Writing pregap tracks...: ");
	write_gap_tracks(outfp);
	printf("OK\n");

	// ecrire la piste 2 head.
	echo("Writing header track....: ");
	write_data_header_boot_track(outfp, infp);
	printf("OK\n");

	// ecrire en tête (le footer)
	echo("Writing CDI header......: ");
	write_data_cdi_header(
		outfp,
		outfilename,
		volume_name,
		data_blocks_count,
		get_total_cdi_space_used(data_blocks_count)
	);
	printf("OK\n");
}

// let's go my friends
int main(int argc, char *argv[]) {
	char outfilename[256];
	FILE *outfp, *infp;

	set_program_name( argv[0] );

	print_head();

	if (argc < 3) {
		print_help();
		return 1;
	}

	infp = fopen(argv[1], "rb");
	if (infp == NULL) {
		fprintf(stderr, "%serror: unable to open source file!", KRED);
		return 2;
	}

	if (check_iso_is_bootable(infp) == 0) {
		fprintf(stderr, "%swarning: %s seems not to be bootable!\n\n", KRED, argv[1]);
	}

	strcpy(outfilename, argv[2]);

	outfp = fopen(outfilename, "wb");
	if (outfp == NULL) {
		fprintf(stderr, "%serror: unable to write destination file!", KRED);
		return 2;
	}

	// choisir la méthode

	if ((argc == 4) && (strcmp(argv[3], "-d") == 0)) {
		create_data_data_image(infp, outfp, outfilename);
	} else {
		create_audio_data_image(infp, outfp, outfilename);
	}

	printf("%s\nWoohoo... All done OK!\nYou can burn it now %s...\n", KCYN, WARNING_MSG);

	fclose(outfp);
	fclose(infp);

	return 0;
}

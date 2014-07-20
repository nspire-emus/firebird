/*
 * main.c
 *
 * This file is part of nspire_emu project.
 * It contains the main entry point for the program.
 * It is responsible for argument parsing and showing the help mensage in case it is needed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

// Prints the usage text given an option struct and a message for every entry
void print_help(struct option *opt, char **help)
{
	int i = 0;

	printf("nspire_emu [options]\n\n");
	while(opt[i].name)
	{
		if(opt[i].flag)
			printf("-%s\t\t\t- %s\n", opt[i].name, help[i]);
		else
			printf("-%c,\t--%s\t- %s\n", opt[i].val, opt[i].name, help[i]);
		i++;
	}
}

// Program entry point...
int main(int argc, char **argv)
{
	int i;	// Argument iteration...
	int option_index = 0;
	static int flag_debug = 0;
	static int flag_large_nand = 0;
	static int flag_large_sdram = 0;
	static int flag_debug_on_warn = 0;
	static int flag_verbosity = 0;
	int port_gdb = 0;
	int port_rgdb = 0;
	int keypad_type = -1;
	int product = 0x0E0;
	uint32_t addr_boot2 = 0x11800000;
	char *path_boot1 = NULL;
	char *path_boot2 = NULL;
	char *path_flash = NULL;
	char *path_commands = NULL;
	char *path_log = NULL;
	char *pre_boot2 = NULL;
	char *pre_diags = NULL;
	char *pre_os = NULL;

	static struct option long_options[] =
	{
		/* Flag options... */
		{ "d",		no_argument,	&flag_debug,		1 },
		{ "n",		no_argument,	&flag_large_nand,	1 },
		{ "r",		no_argument,	&flag_large_sdram,	1 },
		{ "w",		no_argument,	&flag_debug_on_warn,	1 },
		{ "v",		no_argument,	&flag_verbosity,	1 },
		{ "vv",		no_argument,	&flag_verbosity,	2 },
		{ "vvv",	no_argument,	&flag_verbosity,	3 },
		{ "vvvv",	no_argument,	&flag_verbosity,	4 },
		{ "vvvvv",	no_argument,	&flag_verbosity,	5 },
		{ "vvvvvv",	no_argument,	&flag_verbosity,	6 },
		{ "vvvvvvv",	no_argument,	&flag_verbosity,	7 },
		{ "vvvvvvvv",	no_argument,	&flag_verbosity,	8 },

		/* Non flag options... */
		{ "boot1",	required_argument,	0,	'1'},
		{ "boot2",	required_argument,	0,	'2'},
		{ "boot2addr",	required_argument,	0,	'b'},
		{ "flash",	required_argument,	0,	'f'},
		{ "gport",	required_argument,	0,	'g'},
		{ "rport",	required_argument,	0,	'a'},
		{ "commands",	required_argument,	0,	'c'},
		{ "keypad",	required_argument, 	0,	'k'},
		{ "model",	required_argument,	0,	'm'},
		{ "logfile",	required_argument,	0,	'l'},
		{ "pre_boot2",	required_argument,	0,	'q'},
		{ "pre_diags",	required_argument,	0,	'p'},
		{ "pre_os",	required_argument,	0,	'o'},

		/* Terminator... */
		{0, 0, 0, 0}
	};

	static char *help[] =
	{
		"enter debug mode on startup",
		"large nand flash size",
		"large sdram size",
		"enter debugger on warning",
		"verbose",
		"very verbose",
		"very very verbose",
		"...",
		"...",
		"...",
		"...",
		"...",
		"location of boot1 image",
		"location of decompressed boot2 image",
		"boot2 image base address",
		"location of flash image",
		"GDB remote protocol through the TCP port",
		"reverse GDB remote protocol??",
		"GDB command file location",
		"keypad type (2 = TI-84 Plus, 4 = Touchpad)",
		"model (original/CAS+/CX/CM, non-CAS/CAS)",
		"write output to logfile",
		"preload flash with boot2 image",
		"preload flash with diag image",
		"preload flash with OS (tnc/tno)"
	};

	// Print header and parse options...
	printf("nSpire emu v0.7+Ndless patches\n");
	while((i = getopt_long(argc, argv, "1:2:b:f:g:a:c:k:m:l:", long_options, &option_index)) != -1)
		if(i == '?')
		{
			// Getopt already printed a message.
			print_help(long_options, help);
			return 1;
		}
		else
			switch(long_options[option_index].val)
			{
			case '1':
				path_boot1 = optarg;
				break;
			case '2':
				path_boot2 = optarg;
				break;
			case 'b':
				addr_boot2 = strtoul(optarg, NULL, 0);
				if(!addr_boot2)
				{
					printf("Invalid boot2 base addr...");
					print_help(long_options, help);
					return 1;
				}
				break;
			case 'f':
				path_flash = optarg;
				break;
			case 'g':
				port_gdb = atoi(optarg);
				if(!port_gdb)
				{
					printf("Invalid listen port for GDB stub...\n");
					print_help(long_options, help);
					return 1;
				}
				break;
			case 'a':
				port_rgdb = atoi(optarg);
				if(!port_gdb)
				{
					printf("Invalid listen port for GDB stub...\n");
					print_help(long_options, help);
					return 1;
				}
				break;
			case 'c':
				path_commands = optarg;
				break;
			case 'k':
				keypad_type = atoi(optarg);
				if(keypad_type < 0 || keypad_type >= 5)
				{
					printf("Invalid keypad...\n");
					print_help(long_options, help);
					return 1;
				}
				break;
			case 'm':
				switch(toupper(optarg[0]))
				{
				case 'P':
					product = 0x0C0;
					if(optarg[1])	// No extra chars please...
					{
						printf("Wrong calculator model...\n");
						print_help(long_options, help);
						return 1;
					}
					break;
				case 'L':
					product = 0x0D0;
					if(optarg[1])
					{
						printf("Wrong calculator model...\n");
						print_help(long_options, help);
						return 1;
					}
					break;
				case 'X':
					product = 0x100;
					break;
				case 'M':
					product = 0x120;
					break;
				default:
					printf("Wrong calculator model...\n");
					print_help(long_options, help);
					return 1;
				}
				if(toupper(optarg[1]) == 'C')
					product = (product == 0x0E0 ? 0x0C1 : (product - 0x10));
				break;
			case 'l':
				path_log = optarg;
				break;
			case 'q':
				pre_boot2 = optarg;
				break;
			case 'p':
				pre_diags = optarg;
				break;
			case 'o':
				pre_os = optarg;
				break;
			default:
				printf("Unrecongnized option...\n");
				print_help(long_options, help);
				return 1;
			}

	// Check for boot1 or boot2...
	if(!path_boot1 && !path_boot2)
	{
		printf("Please specify a boot1 or a boot2 image...\n");
		print_help(long_options, help);
		return 2;
	}

	if(path_boot1 && path_boot2)
	{
		printf("Only one image should be provide. Either boot1 or boot2...\n");
		print_help(long_options, help);
		return 2;
	}

	// Check for a valid keypad...
	// If /K not used, pick a default keypad type appropriate for the model...
	if(keypad_type == -1)
		switch(product)
		{
		case 0x0C0:
			keypad_type = 0;
			break;
		case 0x0F0:
			keypad_type = 4;
			break;
		default:
			keypad_type = 1;
			break;
		}

	// Check that we don't preload to an existing flash...
	if(path_flash && (pre_boot2 || pre_diags || pre_os))
	{
		printf("Can't preload to an existing flash image\n");
		print_help(long_options, help);
		return 3;
	}

	// Start emulator...
	return emulate(flag_debug, flag_large_nand, flag_large_sdram, flag_debug_on_warn, flag_verbosity, port_gdb, port_rgdb, keypad_type, product, addr_boot2, path_boot1, path_boot2, path_flash, path_commands, path_log, pre_boot2, pre_diags, pre_os);
}

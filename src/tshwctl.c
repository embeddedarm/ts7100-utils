/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2019, Technologic Systems Inc. */

#include <dirent.h> 
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "eval_cmdline.h"
#include "fpga.h"
#include "helpers.h"

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ " - "
  GITCOMMIT;

int model = 0;

void do_info(void)
{
	fpga_init(0x50004000);
	eval_cmd_init();

	printf("MODEL=%X\n", model);
	printf("FPGA_REV=0x%X\n", fpeek32(0x0) >> 16);

	if(model == 0x7100) {
		printf("CPU_OPTS=0x%X\n", eval_cmd("cpu_opts"));
		printf("IO_OPTS=0x%X\n", eval_cmd("io_opts"));
		printf("IO_MODEL=0x%X\n", eval_cmd("io_model"));
	} else if(model == 0x7250) {
		printf("MODEL_OPTS=0x%X\n", (~fpeek32(0x10) >> 17) & 0x1F);
		if(fpeek32(0x10) & 0x10000)
			printf("RAM_MB=512\n");
		else
			printf("RAM_MB=1024\n");
		printf("PCBREV=A\n");
	}
}

static void usage(char **argv) {
	fprintf(stderr,
	  "%s\n\n"
	  "Usage: %s [OPTION] ...\n"
	  "Technologic Systems Hardware access\n"
	  "\n"
	  "  -i, --info             Get info about the SBC\n"
	  "  -a, --address <addr>   Set syscon addr offset for FPGA peek/poke\n"
	  "  -r, --peek16           16bit FPGA syscon read, must pass -a too\n"
	  "  -w, --poke16 <value>   16bit FPGA syscon write, must pass -a too\n"
	  "  -l, --peek32           32bit FPGA syscon read, must pass -a too\n"
	  "  -L, --poke32 <value>   32bit FPGA syscon write, must pass -a too\n"
	  "  -h, --help             This message\n"
	  "\n",
	  copyright, argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int opt_info = 0;
	int opt_peek16 = 0, opt_poke16 = 0, opt_peek32 = 0, opt_poke32 = 0;
	uint32_t opt_address = 0x1, opt_pokeval = 0;

	static struct option long_options[] = {
	  { "info", no_argument, NULL, 'i' },
	  { "help", no_argument, NULL, 'h' },
	  { "address", required_argument, NULL, 'a' },
	  { "peek16", no_argument, NULL, 'r' },
	  { "poke16", required_argument, NULL, 'w' },
	  { "peek32", no_argument, NULL, 'l' },
	  { "poke32", required_argument, NULL, 'L' },
	  { NULL, no_argument, NULL, 0 }
	};

	if(argc == 1) {
		usage(argv);
		return 1;
	}

	model = get_model();
	switch(model) {
	  case 0x7100:
	  case 0x7250:
		break;
	  default:
		fprintf(stderr, "Unsupported model TS-%x\n", model);
		return 1;
	}

	while((c = getopt_long(argc, argv, 
	  "iha:rw:lL:",
	  long_options, NULL)) != -1) {
		switch (c) {
		  case 'i': /* FPGA info */
			opt_info = 1;
			break;
		  case 'a': /* FPGA Address */
			opt_address = strtoul(optarg, NULL, 0);
			break;
		  case 'r': /* FPGA Peek16 */
			opt_peek16 = 1;
			break;
		  case 'w': /* FPGA Poke16 */
			opt_poke16 = 1;
			opt_pokeval = strtoul(optarg, NULL, 0);
			break;
		  case 'l': /* FPGA Peek32 */
			opt_peek32 = 1;
			break;
		  case 'L': /* FPGA Poke32 */
			opt_poke32 = 1;
			opt_pokeval = strtoul(optarg, NULL, 0);
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	if (opt_info) {
		do_info();
	}

	if (opt_peek16 || opt_poke16) {
		if (opt_address & 0x1) {
			error(EFAULT, EFAULT, "Address offset must be 16 bit "
			  "aligned for 16 bit FPGA accesses");
		}

		fpga_init(0x50004000);
		if (opt_poke16) fpoke16(opt_address, opt_pokeval & 0xFFFF);
		if (opt_peek16) printf("0x%04X\n", fpeek16(opt_address));
	}

	if (opt_peek32 || opt_poke32) {
		if (opt_address & 0x3) {
			error(EFAULT, EFAULT, "Address offset must be 32 bit "
			  "aligned for 32 bit FPGA accesses");
		}

		fpga_init(0x50004000);
		if (opt_poke32) fpoke32(opt_address, opt_pokeval);
		if (opt_peek32) printf("0x%08X\n", fpeek32(opt_address));
	}

	return 0;
}

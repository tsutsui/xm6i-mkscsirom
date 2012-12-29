/*-
 * Copyright (c) 2012 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * mkscsirom.c
 *
 * A dumb tool to create pseudo SCSIROM files for XM6i,
 * using XM6Util.exe and FORMAT.x in Human68k floppy image binaries
 */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/endian.h>
#include <sys/stat.h>
#include <sys/types.h>

//#define SCSIIN
//#define SCSIEX
//#define ROM30
#if !defined(SCSIIN) && !defined(SCSIEX) && !defined(ROM30)
#define ROM30	/* default */
#endif

#define XM6UTIL_SIZE		73728		/* XM6Util ver 2.06 */
#define HUMAN302_SIZE		1261568		/* Human68k 3.02 */

#define SCSIIN_BASE		0x00fc0000
#define SCSIIN_OFFSET		0
#define SCSIIN_SIZE		0x2000		/* 8KB */

#define SCSIEX_BASE		0x00ea0000
#define SCSIEX_OFFSET		0x20
#define SCSIEX_SIZE		0x2000		/* 8KB */

#define ROM30_BASE		SCSIIN_BASE
#define ROM30_OFFSET		SCSIIN_OFFSET
#define ROM30_SIZE		(128 * 1024)	/* 128KB */

#define XM6UTIL_SCSIIN_OFFSET	0xa1e8
#define XM6UTIL_SCSIIN_END	0xa547
#define XM6UTIL_SCSIIN_SIZE	(XM6UTIL_SCSIIN_END - XM6UTIL_SCSIIN_OFFSET + 1)

#define XM6UTIL_SCSIEX_OFFSET	0xa568
#define XM6UTIL_SCSIEX_END	0xa8c9
#define XM6UTIL_SCSIEX_SIZE	(XM6UTIL_SCSIEX_END - XM6UTIL_SCSIEX_OFFSET + 1)

#define HUMAN302_IOCS_OFFSET	0x9ec94
#define HUMAN302_IOCS_END	0xa03cb
#define HUMAN302_IOCS_SIZE	(HUMAN302_IOCS_END - HUMAN302_IOCS_OFFSET + 1)

#ifdef SCSIIN
#define SCSIBOOT_OFFSET		XM6UTIL_SCSIIN_OFFSET
#define SCSIBOOT_SIZE		XM6UTIL_SCSIIN_SIZE
#define ROMBASE			SCSIIN_BASE
#define ROMSIZE			SCSIIN_SIZE
#define SCSI_BOOT_OFFSET	0x00
#define SCSI_INIT_OFFSET	0x20
#define HUMAN_TABLE_OFFSET	0x60
#define ROM_SCSIBOOT_OFFSET	0
#define ROM_IOCS_OFFSET		SCSIBOOT_SIZE
#endif

#ifdef SCSIEX
#define SCSIBOOT_OFFSET		XM6UTIL_SCSIEX_OFFSET
#define SCSIBOOT_SIZE		XM6UTIL_SCSIEX_SIZE
#define ROMBASE			SCSIEX_BASE
#define ROMSIZE			SCSIEX_SIZE
#define SCSI_BOOT_OFFSET	(SCSIEX_OFFSET + 0x00)
#define SCSI_INIT_OFFSET	(SCSIEX_OFFSET + 0x20)
#define HUMAN_TABLE_OFFSET	(SCSIEX_OFFSET + 0x62)
#define ROM_SCSIBOOT_OFFSET	SCSIEX_OFFSET
#define ROM_IOCS_OFFSET		(SCSIEX_OFFSET + SCSIBOOT_SIZE)
#endif

#ifdef ROM30
#define SCSIBOOT_OFFSET		XM6UTIL_SCSIIN_OFFSET
#define SCSIBOOT_SIZE		XM6UTIL_SCSIIN_SIZE
#define ROMBASE			ROM30_BASE
#define ROMSIZE			ROM30_SIZE
#define SCSI_BOOT_OFFSET	0x00
#define SCSI_INIT_OFFSET	0x20
#define HUMAN_TABLE_OFFSET	0x60
#define ROM_SCSIBOOT_OFFSET	0
#define ROM_IOCS_OFFSET		SCSIBOOT_SIZE
#endif

static void
usage(void)
{
	const char *prog;

	prog = getprogname();
	fprintf(stderr, "usage: %s XM6Util.exe HUMAN302.XDF romfile\n", prog);
	exit(EXIT_FAILURE);
}

#if 0 /* just for reference */
static uint32_t
be32dec(void *stream)
{
	uint8_t *p;

	p = stream;
	return ((p[0] << 24) | (p[1] << 16) | (p[2] <<  8) | (p[3] <<  0));
}

static void
be32enc(void *stream, uint32_t host32)
{
	uint8_t *p;

	p = stream;

	p[0] = (host32 >> 24) & 0xff;
	p[1] = (host32 >> 16) & 0xff;
	p[2] = (host32 >>  8) & 0xff;
	p[3] = (host32 >>  0) & 0xff;
}
#endif

static void
adjust_vector(void *p, uint32_t rombase)
{

	be32enc(p, be32dec(p) + rombase);
}

int
main(int argc, char **argv)
{
	struct stat sb;
	int xfd, hfd, rfd;
	const char *xm6util, *human302, *scsirom;
	unsigned char *rombuf;
	int count, vector;

	if (argc != 4)
		usage();

	xm6util  = argv[1];
	human302 = argv[2];
	scsirom  = argv[3];

	/*
	 * open and check input files
	 */
	if ((xfd = open(xm6util, O_RDONLY, 0600)) == -1)
		err(EXIT_FAILURE, "can't open XM6Util.exe `%s'", xm6util);
	if (fstat(xfd, &sb) != 0)
		err(EXIT_FAILURE, "can't stat XM6Util.exe");
	if (sb.st_size != XM6UTIL_SIZE)
		err(EXIT_FAILURE, "invalid size of XM6Util.exe (%d)",
		    (int)sb.st_size);

	if ((hfd = open(human302, O_RDONLY, 0600)) == -1)
		err(EXIT_FAILURE, "create open HUMAN302.XDF `%s'", human302);
	if (fstat(hfd, &sb) != 0)
		err(EXIT_FAILURE, "can't stat HUMAN302.XDF");
	if (sb.st_size != HUMAN302_SIZE)
		err(EXIT_FAILURE, "invalid size of HUMAN302.XDF size (%d)",
		    (int)sb.st_size);

	/* allocate buffer for output romfile */
	rombuf = malloc(ROMSIZE);
	if (rombuf == NULL)
		err(1, "malloc output buffer");
	memset(rombuf, 0, ROMSIZE);

	/* read SCSIBOOT code from XM6Util.exe */
	if (lseek(xfd, SCSIBOOT_OFFSET, SEEK_SET) < 0)
		err(EXIT_FAILURE, "lseek: XM6Util.exe");
	assert(ROM_SCSIBOOT_OFFSET + SCSIBOOT_SIZE <= ROMSIZE);
	count = read(xfd, rombuf + ROM_SCSIBOOT_OFFSET, SCSIBOOT_SIZE);
	if (count != SCSIBOOT_SIZE)
		err(EXIT_FAILURE, "failed to read XM6Util.exe");
	close(xfd);

	/* read SCSI IOCS from HUMAN302.XDF */
	if (lseek(hfd, HUMAN302_IOCS_OFFSET, SEEK_SET) < 0)
		err(EXIT_FAILURE, "lseek: HUMAN302.XDF");
	assert(ROM_IOCS_OFFSET + HUMAN302_IOCS_SIZE <= ROMSIZE);
	count = read(hfd, rombuf + ROM_IOCS_OFFSET, HUMAN302_IOCS_SIZE);
	if (count != HUMAN302_IOCS_SIZE)
		err(EXIT_FAILURE, "failed to read HUMAN302.XDF");
	close(hfd);

	/*
	 * adjust SCSI ROM vector offset
	 */

	/* scsi_boot vectors */
	for (vector = 0; vector < 8; vector++)
		adjust_vector(&rombuf[SCSI_BOOT_OFFSET + vector * 4], ROMBASE);

	/* scsi_init vector */
	adjust_vector(&rombuf[SCSI_INIT_OFFSET], ROMBASE);

	/* human_init and scsi_iocs in human_table */
	adjust_vector(&rombuf[HUMAN_TABLE_OFFSET + 0], ROMBASE);
	adjust_vector(&rombuf[HUMAN_TABLE_OFFSET + 4], ROMBASE);

#ifdef ROM30
	/*
	 * patch SCSIBOOT code to make faked ROM30.DAT bootable from SCSI
	 */

	/* make scsi_init vector no-op */
	rombuf[0x0030] = 0x4e;	/* fc0030:	rts			*/
	rombuf[0x0031] = 0x75;

	/* skip initialization of SCSI IOCS vector in scsi_boot */
	rombuf[0x0086] = 0x60;	/* fc0086:	bras 0xfc00b0		*/
	rombuf[0x0087] = 0x28;

	/* use sector size returned by IOCS S_READCAP for S_READEXT */
	rombuf[0x00ee] = 0x7a;	/* fc00ee:	moveq #0,%d5		*/
	rombuf[0x00ef] = 0x00;
	rombuf[0x00f0] = 0x1a;	/* fc00f0:	moveb %a2@(6),%d5	*/
	rombuf[0x00f1] = 0x29;
	rombuf[0x00f2] = 0x00;
	rombuf[0x00f3] = 0x06;
	rombuf[0x00f4] = 0xe2;	/* fc00f4:	lsrb #1,%d5		*/
	rombuf[0x00f5] = 0x0d;
	rombuf[0x00f6] = 0x4e;	/* fc00f6:	nop			*/
	rombuf[0x00f7] = 0x71;

	rombuf[0x00fc] = 0x4e;	/* fc00fc:	nop			*/
	rombuf[0x00fd] = 0x71;

	/*  adjust IPL sector LBA per sector size */
	rombuf[0x011a] = 0x60;	/* fc011a:	bras fc00a2		*/
	rombuf[0x011b] = 0x86;

	rombuf[0x00a2] = 0x74;	/* fc00a2:	moveq #2,%d2		*/
	rombuf[0x00a3] = 0x02;
	rombuf[0x00a4] = 0x4c;	/* fc00a4:	divul %d5,%d2		*/
	rombuf[0x00a5] = 0x45;
	rombuf[0x00a6] = 0x20;
	rombuf[0x00a7] = 0x02;
	rombuf[0x00a8] = 0x4a;	/* fc00a8:	tstl %d2		*/
	rombuf[0x00a9] = 0x82;
	rombuf[0x00aa] = 0x66;	/* fc00aa:	bnes fc00ae		*/
	rombuf[0x00ab] = 0x02;
	rombuf[0x00ac] = 0x52;	/* fc00ac:	addql #1,%d2		*/
	rombuf[0x00ad] = 0x82;
	rombuf[0x00ae] = 0x60;	/* fc00ae:	bras fc011c		*/
	rombuf[0x00af] = 0x6c;

	/* skip initialization of SCSI IOCS vector in human_init */
	rombuf[0x0168] = 0x60;	/* fc0168:	bras 0xfc0198		*/
	rombuf[0x0169] = 0x2e;
#endif

#ifdef SCSIEX
	/* patch SPC address in SCSI IOCS */
	rombuf[SCSIEX_OFFSET + 0x0497] = 0xea;
	rombuf[SCSIEX_OFFSET + 0x0498] = 0x00;
	rombuf[SCSIEX_OFFSET + 0x0499] = 0x00;
#endif

	/* open and write output romfile */
	if ((rfd = open(scsirom, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
		err(EXIT_FAILURE, "create output romfile `%s'", scsirom);
	
	count = write(rfd, rombuf, ROMSIZE);
	if (count != ROMSIZE)
		err(EXIT_FAILURE, "failed to write %s", scsirom);
	close(rfd);

	free(rombuf);

	printf("%s is written successfully\n", scsirom);

	exit(EXIT_SUCCESS);
}

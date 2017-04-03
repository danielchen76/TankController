/*
 * fs.c
 *
 *  Created on: Apr 13, 2015
 *      Author: Daniel
 */


#include <spiffs.h>
#include <tc_spi.h>
#include "spi_flash.h"

#define LOG_PAGE_SIZE       256

static u8_t spiffs_work_buf[LOG_PAGE_SIZE*2];
static u8_t spiffs_fds[32*4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE+32)*4];

spiffs fs;

static s32_t fs_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
	configASSERT(size <= 256);

	sFLASH_ReadBuffer(dst, addr, (u16_t)size);

	return SPIFFS_OK;
}

static s32_t fs_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
	sFLASH_WritePage(src, addr, (u16_t)size);
	// my_spi_write(addr, size, dst);
	return SPIFFS_OK;
}

static s32_t fs_spiffs_erase(u32_t addr, u32_t size)
{
	configASSERT(size == 4 * 1024);

	// earse sector
	sFLASH_Erase4KSector(addr);

	return SPIFFS_OK;
}

void InitSpiffs( void )
{
	spiffs_config cfg;

	/* Initialize the SPI FLASH driver */
	sFLASH_Init();

	/* Get SPI Flash ID */
//	FlashID = sFLASH_ReadID();

//	cfg.phys_size = 16 * 1024 * 1024; 	// use all spi flashn (16MB spi flash)
//	cfg.phys_addr = 0; 					// start spiffs at start of spi flash
//	cfg.phys_erase_block = 4 * 1024; 	// according to datasheet
//	cfg.log_block_size = 4 * 1024; 		// let us not complicate things
//	cfg.log_page_size = LOG_PAGE_SIZE; 	// as we said

	cfg.hal_read_f = fs_spiffs_read;
	cfg.hal_write_f = fs_spiffs_write;
	cfg.hal_erase_f = fs_spiffs_erase;

	SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
			sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), 0);
	//printf("mount res: %i\n", res);
}

void test_spiffs()
{
	char buf[12];
	char buf1[12];

	// Surely, I've mounted spiffs before entering here

	spiffs_file fd = SPIFFS_open(&fs, "my_file",
			SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	if (SPIFFS_write(&fs, fd, (u8_t *) "Hello world", 12) < 0)
	{
		//printf("errno %i\n", SPIFFS_errno(&fs));
		buf[0] = 0;
	}
	SPIFFS_close(&fs, fd);

	memset(buf1, 0, sizeof(buf1));

	fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
	if (SPIFFS_read(&fs, fd, (u8_t *) buf1, 12) < 0)
	{
		//printf("errno %i\n", SPIFFS_errno(&fs));
		buf[0] = 0;
	}
	SPIFFS_close(&fs, fd);

	//printf("--> %s <--\n", buf);
	buf[0] = 0;
}


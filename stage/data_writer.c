#include <stdio.h>
#include <direct.h>

extern unsigned char binary_igor_2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf_dylib_start[];
extern unsigned int binary_igor_2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf_dylib_end[];

extern unsigned char binary_igor_3e404d11fcbd5486d3be2dd86ce21316e1854842_dylib_start[];
extern unsigned int binary_igor_3e404d11fcbd5486d3be2dd86ce21316e1854842_dylib_end[];

extern unsigned char binary_igor_74227c0021c5e12effb5bd3175eb469a8df0622e_dylib_start[];
extern unsigned int binary_igor_74227c0021c5e12effb5bd3175eb469a8df0622e_dylib_end[];

extern unsigned char binary_igor_b735701843456754988021d128c2671ee36d1b04_dylib_start[];
extern unsigned int binary_igor_b735701843456754988021d128c2671ee36d1b04_dylib_end[];

extern unsigned char binary_igor_f6c17e934ba0ad477812de0b7cb019396d259d93_dylib_start[];
extern unsigned int binary_igor_f6c17e934ba0ad477812de0b7cb019396d259d93_dylib_end[];

extern unsigned char binary_igor_install_start[];
extern unsigned int binary_igor_install_end[];

extern unsigned char binary_igor_map_plist_start[];
extern unsigned int binary_igor_map_plist_end[];

extern unsigned char binary_resources_1024x768_jpg_start[];
extern unsigned int binary_resources_1024x768_jpg_end[];

extern unsigned char binary_resources_320x480_jpg_start[];
extern unsigned int binary_resources_320x480_jpg_end[];

extern unsigned char binary_resources_icon_ico_start[];
extern unsigned int binary_resources_icon_ico_end[];

extern unsigned char binary_resources_overrides_plist_start[];
extern unsigned int binary_resources_overrides_plist_end[];

extern unsigned char binary_dl_dl_exe_start[];
extern unsigned int binary_dl_dl_exe_end[];

void write_stuff() {
	_mkdir("igor");
	{
		FILE *fp = fopen("igor/2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf.dylib", "wb");
	
fwrite(binary_igor_2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf_dylib_start, 1, (size_t) &binary_igor_2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf_dylib_end - (size_t) &binary_igor_2dcde0a77381d24b7c02ac0cf7f714434c4ccdcf_dylib_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/3e404d11fcbd5486d3be2dd86ce21316e1854842.dylib", "wb");
	
fwrite(binary_igor_3e404d11fcbd5486d3be2dd86ce21316e1854842_dylib_start, 1, (size_t) &binary_igor_3e404d11fcbd5486d3be2dd86ce21316e1854842_dylib_end - (size_t) &binary_igor_3e404d11fcbd5486d3be2dd86ce21316e1854842_dylib_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/74227c0021c5e12effb5bd3175eb469a8df0622e.dylib", "wb");
	
fwrite(binary_igor_74227c0021c5e12effb5bd3175eb469a8df0622e_dylib_start, 1, (size_t) &binary_igor_74227c0021c5e12effb5bd3175eb469a8df0622e_dylib_end - (size_t) &binary_igor_74227c0021c5e12effb5bd3175eb469a8df0622e_dylib_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/b735701843456754988021d128c2671ee36d1b04.dylib", "wb");
	
fwrite(binary_igor_b735701843456754988021d128c2671ee36d1b04_dylib_start, 1, (size_t) &binary_igor_b735701843456754988021d128c2671ee36d1b04_dylib_end - (size_t) &binary_igor_b735701843456754988021d128c2671ee36d1b04_dylib_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/f6c17e934ba0ad477812de0b7cb019396d259d93.dylib", "wb");
	
fwrite(binary_igor_f6c17e934ba0ad477812de0b7cb019396d259d93_dylib_start, 1, (size_t) &binary_igor_f6c17e934ba0ad477812de0b7cb019396d259d93_dylib_end - (size_t) &binary_igor_f6c17e934ba0ad477812de0b7cb019396d259d93_dylib_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/install", "wb");
	
fwrite(binary_igor_install_start, 1, (size_t) &binary_igor_install_end - (size_t) &binary_igor_install_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("igor/map.plist", "wb");
	
fwrite(binary_igor_map_plist_start, 1, (size_t) &binary_igor_map_plist_end - (size_t) &binary_igor_map_plist_start, fp);
		fclose(fp);
	}
	_mkdir("resources");
	{
		FILE *fp = fopen("resources/1024x768.jpg", "wb");
	
fwrite(binary_resources_1024x768_jpg_start, 1, (size_t) &binary_resources_1024x768_jpg_end - (size_t) &binary_resources_1024x768_jpg_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("resources/320x480.jpg", "wb");
	
fwrite(binary_resources_320x480_jpg_start, 1, (size_t) &binary_resources_320x480_jpg_end - (size_t) &binary_resources_320x480_jpg_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("resources/icon.ico", "wb");
	
fwrite(binary_resources_icon_ico_start, 1, (size_t) &binary_resources_icon_ico_end - (size_t) &binary_resources_icon_ico_start, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen("resources/overrides.plist", "wb");
	
fwrite(binary_resources_overrides_plist_start, 1, (size_t) &binary_resources_overrides_plist_end - (size_t) &binary_resources_overrides_plist_start, fp);
		fclose(fp);
	}
	_mkdir("dl");
	{
		FILE *fp = fopen("dl/dl.exe", "wb");
	
fwrite(binary_dl_dl_exe_start, 1, (size_t) &binary_dl_dl_exe_end - (size_t) &binary_dl_dl_exe_start, fp);
		fclose(fp);
	}
}


#include <os.h>
#include "unzip.h"
#include <nspireio.h>

#define DEBUG_CONSOLE 0
const char PACSPIRE_ROOT[] = "/pacspire";

typedef struct
{
	char extension[15];
	char program[15];
} fileext;

typedef struct
{
	char name[30];
	char program[15];
} link;

typedef struct
{
	char name[21];
	char version[11];
	unsigned int timestamp;
	
	int ext_count;
	fileext* extensions;
	
	int link_count;
	link* links;
} pkginfo;


#define debug(s, ...) \
	nio_printf(s, ##__VA_ARGS__)
#define success(s, ...) \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_GREEN); \
	nio_printf(s, ##__VA_ARGS__); \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_BLACK);
#define warn(s, ...) \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_YELLOW); \
	nio_printf(s, ##__VA_ARGS__); \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_BLACK);
#define fail(s, ...) \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_RED); \
	nio_printf(s, ##__VA_ARGS__); \
	nio_color(nio_get_default(),NIO_COLOR_WHITE,NIO_COLOR_BLACK);

void freePackageInfo(pkginfo* p)
{
	free(p->extensions);
	free(p->links);
	free(p);
}
	
pkginfo* parsePackageInfo(char* buffer)
{
	char* line;
	pkginfo* p = malloc(sizeof(pkginfo));
	p->name[0] = '\0';
	p->version[0] = '\0';
	p->timestamp = 0;
	p->ext_count = 0;
	p->extensions = NULL;
	p->link_count = 0;
	p->links = NULL;
	
	line = strtok(buffer,"\r\n");
	while(line != NULL)
	{
		int len = strlen(line);
		
		int delimiter_pos = strcspn(line,"=");
		if(delimiter_pos == len)
		{
			freePackageInfo(p);
			return NULL;
		}
		
		line[delimiter_pos] = '\0';
		if(strcmp(line, "name") == 0)
		{
			strncpy(p->name,&line[delimiter_pos+1],20);
			p->name[20] = '\0';
		}
		else if(strcmp(line, "version") == 0)
		{
			strncpy(p->version,&line[delimiter_pos+1],10);
			p->name[10] = '\0';
		}
		else if(strcmp(line, "timestamp") == 0)
		{
			p->timestamp = (unsigned int)strtoul(&line[delimiter_pos+1],NULL,0);
			if(p->timestamp == 0)
			{
				freePackageInfo(p);
				return NULL;
			}
		}
		else if(strcmp(line, "ext_name") == 0)
		{
			p->ext_count++;
			p->extensions = realloc(p->extensions,p->ext_count*sizeof(fileext));
			strncpy(p->extensions[p->ext_count-1].extension,&line[delimiter_pos+1],15);
		}
		else if(strcmp(line, "ext_prog") == 0)
		{
			strncpy(p->extensions[p->ext_count-1].program,&line[delimiter_pos+1],15);
		}
		else if(strcmp(line, "link_name") == 0)
		{
			p->link_count++;
			p->links = realloc(p->links,p->link_count*sizeof(link));
			strncpy(p->links[p->link_count-1].name,&line[delimiter_pos+1],30);
		}
		else if(strcmp(line, "link_prog") == 0)
		{
			strncpy(p->links[p->link_count-1].program,&line[delimiter_pos+1],15);
		}
		else
		{
			freePackageInfo(p);
			return NULL;
		}
		
		line = strtok(NULL,"\r\n");
	}
	
	if(p->name[0] == '\0' || p->version[0] == '\0' || p->timestamp == 0)
	{
		freePackageInfo(p);
		return NULL;
	}
	
	return p;
}

void* unzGetCurrentFileContent(unzFile uf)
{
	unz_file_info file_info;
	unzGetCurrentFileInfo(uf,&file_info,NULL,0,NULL,0,NULL,0);
	
	if(unzOpenCurrentFile(uf) != UNZ_OK)
		return NULL;
	
	char* buffer = malloc(file_info.uncompressed_size+1);
	if(buffer == NULL)
	{
		unzCloseCurrentFile(uf);
		return NULL;
	}
	
	if(unzReadCurrentFile(uf,buffer,file_info.uncompressed_size) < 0)
	{
		unzCloseCurrentFile(uf);
		return NULL;
	}
	buffer[file_info.uncompressed_size] = 0;
	unzCloseCurrentFile(uf); // Detect CRC errors?
	
	return buffer;
}

void* unzGetFileContent(unzFile uf, const char* filename)
{
	if(unzLocateFile(uf,filename,1) != UNZ_OK)
		return NULL;
	
	return unzGetCurrentFileContent(uf);
}

void* getFileContent(const char* filename)
{
	struct stat s;
	if(stat(filename,&s) == -1)
		return NULL;
	
	FILE* f = fopen(filename,"rb");
	if(f == NULL)
		return NULL;
	
	void* buffer = malloc(s.st_size);
	if(buffer == NULL)
	{
		fclose(f);
		return NULL;
	}
	
	if(fread(buffer,1,s.st_size,f) == 0)
	{
		fclose(f);
		free(buffer);
		return NULL;
	}
		
	fclose(f);
	return buffer;
}

int writeFileContent(const char* filename, void* buffer, size_t count)
{
	FILE* f = fopen(filename,"wb");
	if(f == NULL)
		return -1;
		
	if(fwrite(buffer,1,count,f) != count)
	{
		fclose(f);
		return -1;
	}
	
	fclose(f);
	return 0;
}

int createDir(const char *dir) {
	char tmp[100];
	char *p = NULL;
	size_t len;

	strncpy(tmp,dir,100);
	len = strlen(tmp);
	if(tmp[len - 1] == '/' || tmp[len - 1] == '\\')
		tmp[len - 1] = '\0';
		
	for(p = tmp + 1; *p; p++)
	{
		if(*p == '/' || *p == '\\') {
			*p = '\0';
			if(mkdir(tmp, 0755) == -1 && errno != 17)
				return -1;
			*p = '/';
		}
	}
	if(mkdir(tmp, 0755) == -1 && errno != 17)
		return -1;
		
	return 0;
}

int removeDir(char* directory)
{
	DIR* dir;
	struct dirent* entry;
	
	dir = opendir(directory);
	if(dir == NULL)
		return -1;
		
	while((entry = readdir(dir)) != 0)
	{
		if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
			continue;
		
		char full_path[100];
		strcpy(full_path,directory);
		strcat(full_path,"/");
		strcat(full_path,entry->d_name);
		
		struct stat s;
		if(stat(full_path,&s) == -1)
		{
			closedir(dir);
			return -1;
		}
		
		if(s.st_mode & S_IFDIR)
		{
			if(removeDir(full_path) == -1)
			{
				closedir(dir);
				return -1;
			}
		}
		else
		{
			if(unlink(full_path) == -1)
			{
				closedir(dir);
				return -1;
			}
		}
	}
	
	closedir(dir);
	if(rmdir(directory) == -1)
		return -1;
	
	return 0;
}

enum
{
	INSTALLATION_SUCCESS,
	INSTALLATION_FAILED,
	INSTALLATION_ABORTED
};

int installPackage(char* file)
{
	unzFile uf = NULL;
	
	debug("opening package...");
	uf = unzOpen(file);
	if(uf == NULL)
	{
		fail(" failed\n");
		return INSTALLATION_FAILED;
	}
	success(" done\n");
	
	debug("unzipping package info...");
	void* buffer = unzGetFileContent(uf,"pkginfo.txt.tns");
	if(buffer == NULL)
	{
		fail(" failed\n");
		unzClose(uf);
		free(buffer);
		return INSTALLATION_FAILED;
	}
	success(" done\n");
	
	debug("parsing package info...");
	pkginfo* p = parsePackageInfo(buffer);
	free(buffer);
	if(p == NULL)
	{
		fail(" failed\n");
		unzClose(uf);
		return INSTALLATION_FAILED;
	}
	success(" done\n");
	
	debug("Package %s\n",p->name);
	debug("Version: %s\n",p->version);
	debug("Timestamp: %d\n",p->timestamp);
	
	debug("checking if package is already installed...");
	char full_path[50];
	sprintf(full_path,"%s/%s",PACSPIRE_ROOT,p->name);
	struct stat s;
	if(stat(full_path,&s) == 0)
	{
		warn(" yes\n");
		
		debug("opening installed pkginfo.txt.tns...");
		char pkginfo_path[60];
		sprintf(pkginfo_path,"%s/pkginfo.txt.tns",full_path);
		buffer = getFileContent(pkginfo_path);
		if(buffer == NULL)
		{
			fail(" failed\n");
			freePackageInfo(p);
			unzClose(uf);
			return INSTALLATION_FAILED;
		}
		success(" done\n");
		
		debug("parsing installed pkginfo.txt.tns...");
		pkginfo* p2 = parsePackageInfo(buffer);
		free(buffer);
		if(p2 == NULL)
		{
			fail(" failed\n");
			freePackageInfo(p);
			unzClose(uf);
			return INSTALLATION_FAILED;
		}
		success(" done\n");
		
		debug("checking if the package is newer than the installed version...");
		char message[200];
		if(p->timestamp <= p2->timestamp)
		{
			warn(" no\n");
			sprintf(message,"You already have a newer or the same version of %s installed.",p->name);
			if(show_msgbox_2b("pacspire",message,"OK","Force installation") == 1)
			{
				fail("Installation aborted\n");
				freePackageInfo(p);
				freePackageInfo(p2);
				unzClose(uf);
				return INSTALLATION_ABORTED;
			}
		}
		else
		{
			success(" yes\n");
			sprintf(message,"Do you want to update %s (%s -> %s)?",p->name,p2->version,p->version);
			if(show_msgbox_2b("pacspire",message,"Yes","No") == 2)
			{
				fail("Installation aborted by user\n");
				freePackageInfo(p);
				freePackageInfo(p2);
				unzClose(uf);
				return INSTALLATION_ABORTED;
			}
		}
		
		freePackageInfo(p2);
		
		debug("Removing previous installation...");
		if(removeDir(full_path) == -1)
		{
			debug(" failed\n");
			freePackageInfo(p);
			unzClose(uf);
			return INSTALLATION_FAILED;
		}
		success(" done\n");
	}
	else
	{
		success(" no\n");
		char message[50];
		sprintf(message,"Do you want to install %s?",p->name);
		if(show_msgbox_2b("pacspire",message,"Install","Cancel") == 2)
		{
			fail("Installation aborted by user\n");
			freePackageInfo(p);
			unzClose(uf);
			return INSTALLATION_ABORTED;
		}
	}
	
	debug("Creating directory %s...",full_path);
	if(mkdir(full_path, 0755) == -1)
	{
		if(errno == 17) // == EEXIST
		{
			warn(" exists\n");
		}
		else
		{
			fail(" failed\n");
			freePackageInfo(p);
			unzClose(uf);
			return INSTALLATION_FAILED;
		}
	}
	success(" done\n");
	
	debug("Going to first file in zip...");
	if(unzGoToFirstFile(uf) != UNZ_OK)
	{
		fail(" failed\n");
		freePackageInfo(p);
		unzClose(uf);
		return INSTALLATION_FAILED;
	}
	success(" done\n");
	
	do
	{
		char filename[50];
		unz_file_info file_info;
		unzGetCurrentFileInfo(uf,&file_info,filename,50,NULL,0,NULL,0);
		
		if(filename[strlen(filename)-1] == '/')
		{
			debug("Creating directory %s...",filename);
			char dir_path[60];
			sprintf(dir_path,"%s/%s",full_path,filename);
			if(createDir(dir_path) == -1)
			{
				fail(" failed\n");
				freePackageInfo(p);
				unzClose(uf);
				return INSTALLATION_FAILED;
			}
			success(" done\n");
		}
		else
		{
			debug("Inflating file %s...",filename);
			char* buffer = unzGetCurrentFileContent(uf);
			if(buffer == NULL)
			{
				fail(" failed\n");
				freePackageInfo(p);
				unzClose(uf);
				return INSTALLATION_FAILED;
			}
			success(" done\n");
			
			debug("Writing content to file...");
			char file_path[60];
			sprintf(file_path,"%s/%s",full_path,filename);
			if(writeFileContent(file_path,buffer,file_info.uncompressed_size) == -1)
			{
				fail(" failed\n");
				free(buffer);
				freePackageInfo(p);
				unzClose(uf);
				return INSTALLATION_FAILED;
			}
			success(" done\n");
			
			free(buffer);
		}
	}
	while(unzGoToNextFile(uf) == UNZ_OK);
	
	if(p->ext_count > 0)
	{
		int i;
		for(i = 0; i < p->ext_count; i++)
		{
			debug("Registering extension %s for %s...",p->extensions[i].extension,p->extensions[i].program);
			cfg_register_fileext(p->extensions[i].extension,p->extensions[i].program);
			success(" done\n");
		}
	}
	
	if(p->link_count > 0)
	{
		int i;
		for(i = 0; i < p->link_count; i++)
		{
			debug("Creating link (%s.lnk.tns -> %s)...",p->links[i].name,p->links[i].program);
			char link_path[60];
			char exec_path[60];
			
			sprintf(link_path,"/documents/%s.lnk.tns",p->links[i].name);
			sprintf(exec_path,"%s/%s",full_path,p->links[i].program);
			if(writeFileContent(link_path,exec_path,strlen(exec_path)) == -1)
			{
				fail(" failed\n");
				freePackageInfo(p);
				unzClose(uf);
				return INSTALLATION_FAILED;
			}
			success(" done\n");
		}
	}
	
	freePackageInfo(p);
	unzClose(uf);
	return INSTALLATION_SUCCESS;
}

int main(int argc, char** argv)
{
	assert_ndless_rev(877);
	
	nio_console c;
	#if DEBUG_CONSOLE == 1
	clrscr();
	nio_init(&c,NIO_MAX_COLS,NIO_MAX_ROWS,0,0,NIO_COLOR_WHITE,NIO_COLOR_BLACK,TRUE);
	#else
	nio_init(&c,NIO_MAX_COLS,NIO_MAX_ROWS,0,0,NIO_COLOR_WHITE,NIO_COLOR_BLACK,FALSE);
	#endif
	nio_set_default(&c);
	
	debug("pacspire (%s %s)\n",__DATE__,__TIME__);
	
	if(argc > 1)
	{
		if(strstr(argv[1],".pcs.tns") != NULL)
		{
			debug("attempting to install package %s\n",argv[1]);
			switch(installPackage(argv[1]))
			{
				case INSTALLATION_ABORTED:
					break;
				
				case INSTALLATION_SUCCESS:
					show_msgbox("pacspire","The installation was successful.");
					break;
					
				case INSTALLATION_FAILED:
					if(show_msgbox_2b("pacspire","The installation failed. Read the log for more details.","OK","View Log") == 2)
					{
						clrscr();
						debug("Press any key to exit...");
						nio_fflush(&c);
						wait_key_pressed();
					}
					break;
			}
		}
		else if(strstr(argv[1],".lnk.tns") != NULL)
		{
			char* exec_path = getFileContent(argv[1]);
			debug("passing control to %s\n",exec_path);
			int ret = nl_exec(exec_path,0,NULL);
			debug("%s returned with status code %d\n",exec_path,ret);
			wait_key_pressed();
		}
	}
	else
	{
		debug("creating PACSPIRE_ROOT directory (%s)...",PACSPIRE_ROOT);
		if(createDir(PACSPIRE_ROOT) == -1)
		{
			fail(" failed\n");
		}
		success(" done\n");
		
		debug("registering .pcs extension...");
		cfg_register_fileext("pcs","pacspire");
		success(" done\n");
		
		debug("registering .lnk extension...");
		cfg_register_fileext("lnk","pacspire");
		success(" done\n");
		show_msgbox("pacspire","pacspire has been installed. Click on a package to install it.");
	}
	
	nio_free(&c);
	refresh_osscr();
	return 0;
}
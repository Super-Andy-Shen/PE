#include<iostream>
#include<Windows.h>
#include <stdio.h>
using namespace std;
// readfile(path string,buffer pointer) -----> read file, write file in heap, return size of file.
DWORD readfile(const char* path, LPVOID* filebuffer)
{
	FILE* file = nullptr;
	file = fopen(path, "rb");
	if (!file)
	{
		cout << "cannot open file at: " << path << endl;
		fclose(file);
		return 1;
	}
	// find the size of file
	fseek(file, 0, SEEK_END);
	DWORD filesize = ftell(file);


	fseek(file, 0, SEEK_SET); //pointer return to the original position,

	//allocate dynamic memory to store the file
	VOID* file_copy = new int[filesize];
	if (!file_copy)
	{
		cout << "failed to allocate memory." << endl;
		fclose(file);
		return 1;
	}
	DWORD n = fread(file_copy, filesize, 1, file);
	if (!n)
	{
		cout << "failed to read" << endl;
		delete[] file_copy;
		fclose(file);
		return 1;
	}
	*filebuffer = file_copy;
	fclose(file);
	return filesize;
}

//printfile(path string)
void printfile(const char* path)
{
	LPVOID filebuffer;
	if (!readfile(path, &filebuffer))
	{
		cout << "failed to readfile at address: " << path << endl;
	}
	else
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
		cout << "e_magic :" << hex << uppercase << dos_header->e_magic << endl;
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*) ((DWORD)filebuffer + dos_header->e_lfanew);

		IMAGE_FILE_HEADER* file_header = (IMAGE_FILE_HEADER*) ((DWORD)nt_header + 4);
		cout << file_header->SizeOfOptionalHeader << endl;

		IMAGE_OPTIONAL_HEADER* optional_header = (IMAGE_OPTIONAL_HEADER*)((DWORD)file_header + sizeof(IMAGE_FILE_HEADER));
		
		//cout << optional_header->BaseOfCode << endl;
		//IMAGE_SECTION_HEADER* section_header =(IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
		
		/*for (auto i = 0; i < file_header->NumberOfSections; i++)
		{
			cout << "Name :" << section_header[i].Name << endl;
		}*/
		return;
	}
}


int main()
{
	char path[] = "C:\\PE\\PETool.exe";
	printfile(path);
	return 0;
}

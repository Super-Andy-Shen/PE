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
		return;
	}
	else
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
		cout << "e_magic :" << hex << uppercase << dos_header->e_magic << endl;
		IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*) ((DWORD)filebuffer + dos_header->e_lfanew);

		IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
			//(IMAGE_FILE_HEADER*) ((DWORD)nt_header + 4);

		IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
			//(IMAGE_OPTIONAL_HEADER*)((DWORD)file_header + sizeof(IMAGE_FILE_HEADER));
		
		IMAGE_SECTION_HEADER* section_header =(IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
		
		for (auto i = 0; i < file_header->NumberOfSections; i++)
		{
			cout << "Name :" << section_header[i].Name << endl;
		}
		delete[] filebuffer;
		return;
	}
}
DWORD RVA_to_FOA(void* imagebuffer, DWORD rva, IMAGE_SECTION_HEADER* section_header, IMAGE_FILE_HEADER* file_header)
{
	DWORD num_sections = file_header->NumberOfSections;
	DWORD rva_offset, foa = 0;
	cout << "hi" << endl;
	for (int i = 0; i < num_sections; i++)
	{
		if ((rva > section_header[i].VirtualAddress) && (rva < section_header[i].VirtualAddress + section_header[i].Misc.VirtualSize))
		{
			rva_offset = rva - section_header[i].VirtualAddress;
			foa = section_header[i].PointerToRawData + rva_offset;
			return foa;
		}
	}
	return foa;
}
void filebuffer_to_imagebuffer_to_disk(const char* path)
{
	LPVOID filebuffer;
	DWORD filesize = readfile(path, &filebuffer);
	if (!filesize)
	{
		cout << "failed to readfile at address: " << path << endl;
		return;
	}
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((DWORD)filebuffer + dos_header->e_lfanew);
	IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
	IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
	// memory allocation
	DWORD image_size = optional_header->SizeOfImage;
	VOID* imagebuffer = new int[image_size];
	if (!imagebuffer)
	{
		cout << "failed to allocate memory." << endl;
		return;
	}
	//fillbuffer -> imagebuffer
	DWORD headers_size = optional_header->SizeOfHeaders;
	memcpy(imagebuffer, dos_header, headers_size);//copy headers;
	DWORD num_sections = file_header->NumberOfSections;
	for (int i = 0; i < num_sections;i++)
	{
		void* source = (void*)((DWORD)dos_header + section_header[i].PointerToRawData);
		void* dest = (void*)((DWORD)imagebuffer + section_header[i].VirtualAddress);
		memcpy(dest,source, section_header[i].SizeOfRawData);
	}
	//imagebuffer -> newbuffer -> disk
	VOID* newbuffer = new int[filesize];
	if (!newbuffer)
	{
		cout << "failed to allocate memory." << endl;
		return;
	}
	memcpy(newbuffer, imagebuffer, headers_size); //copy headers;
	for (int i = 0; i < num_sections; i++)
	{
		void* source = (void*)((DWORD)imagebuffer + section_header[i].VirtualAddress);
		void* dest = (void*)((DWORD)newbuffer + section_header[i].PointerToRawData);
		memcpy(dest, source, section_header[i].SizeOfRawData);// not too sure 如果我们在imagebuffer里面植入代码，sizeofrawdata不够怎么办
	}
	FILE* file = nullptr;
	file = fopen("C:\\PE\\petoolcopy.exe", "wb");
	fwrite(newbuffer, filesize, 1, file);
	fclose(file);


	DWORD rva = 0x1234;
	DWORD foa = RVA_to_FOA(imagebuffer, rva, section_header, file_header);
	if (foa != -1) cout << "when rva is " <<rva<<" foa is : " << foa << endl;
	delete[] filebuffer;
	delete[] imagebuffer;
	delete[] newbuffer;
	return;
}


int main()
{
	char path[] = "C:\\PE\\PETool.exe";
	//printfile(path);
	filebuffer_to_imagebuffer_to_disk(path);
	return 0;
}

#include<iostream>
#include<Windows.h>
#include <stdio.h>
using namespace std;
// readfile(path string,buffer pointer) -----> read file, write file in heap, return size of file.
int merge_section(LPVOID filebuffer, VOID* imagebuffer);
int enlarge_section(LPVOID filebuffer, VOID* enlarge_buffer);
int addnewsection(VOID* imagebuffer, IMAGE_DOS_HEADER* dos_header, IMAGE_FILE_HEADER* file_header, IMAGE_OPTIONAL_HEADER* optional_header, IMAGE_SECTION_HEADER* section_header);
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
DWORD RVA_to_FOA(DWORD rva, IMAGE_SECTION_HEADER* section_header, IMAGE_FILE_HEADER* file_header)
{
	DWORD num_sections = file_header->NumberOfSections;
	DWORD rva_offset, foa = 0;
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
	//addnewsection(imagebuffer, dos_header, file_header, optional_header, section_header);
	//扩大一个节
	// enlarge_section(filebuffer, imagebuffer);
	//合并节
	//merge_section(filebuffer, imagebuffer);
	//imagebuffer -> newbuffer -> disk
	int newbuffer_size = section_header[num_sections - 1].VirtualAddress + section_header[num_sections - 1].Misc.VirtualSize;
	VOID* newbuffer = new int[newbuffer_size];
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
	DWORD new_filesize = filesize;
	FILE* file = nullptr;
	file = fopen("C:\\PE\\PEcopy.exe", "wb");
	fwrite(newbuffer, new_filesize, 1, file);
	fclose(file);
	delete[] filebuffer;
	delete[] imagebuffer;
	delete[] newbuffer;
	return;
}
DWORD ali(DWORD& n,DWORD alig)
{
	if (n % alig == 0) return n / alig;
	else return (n / alig) + 1;
}
int addnewsection(VOID* imagebuffer, IMAGE_DOS_HEADER* dos_header, IMAGE_FILE_HEADER* file_header, IMAGE_OPTIONAL_HEADER* optional_header, IMAGE_SECTION_HEADER* section_header)
{

	DWORD num_section = file_header->NumberOfSections;
	IMAGE_SECTION_HEADER* new_section = section_header;
	for (int i = 0; i < num_section; i++) new_section++;
	DWORD ava_space = optional_header->SizeOfHeaders - ((DWORD) new_section - (DWORD)dos_header);
	if (ava_space < 2 * sizeof(IMAGE_SECTION_HEADER))
	{
		cout << "not enough space" << endl;
		return 1;
	}
	//add a new section header
	void* dest = (void*)((DWORD)imagebuffer + ((DWORD)new_section - (DWORD)dos_header));
	memcpy(dest, section_header, sizeof(IMAGE_SECTION_HEADER));
	//modify file header numerofsection
	DWORD file_offset = ((DWORD)file_header - (DWORD)dos_header);
	IMAGE_FILE_HEADER* fheader = (IMAGE_FILE_HEADER*)((DWORD)imagebuffer + file_offset);
	fheader->NumberOfSections += 1;
	int new_num_section = num_section + 1;	
	//change section virtual address
	DWORD section_offset = ((DWORD)section_header - (DWORD)dos_header);
	IMAGE_SECTION_HEADER* sheader = (IMAGE_SECTION_HEADER*)((DWORD)imagebuffer + section_offset);
	DWORD align = sheader[new_num_section - 2].VirtualAddress + sheader[new_num_section - 2].Misc.VirtualSize;
	DWORD quo = ali(align,optional_header->SectionAlignment);
	sheader[new_num_section - 1].VirtualAddress = quo * optional_header->SectionAlignment;
	//change section virtual size
	sheader[new_num_section - 1].Misc.VirtualSize = 0x1000;
	//change raw size
	sheader[new_num_section - 1].SizeOfRawData = 0x1000;
	//change raw address
	DWORD align_raw = sheader[new_num_section - 2].PointerToRawData + sheader[new_num_section - 2].SizeOfRawData;
	DWORD quo_raw = ali(align_raw, optional_header->FileAlignment);
	sheader[new_num_section - 1].PointerToRawData = quo_raw * optional_header->FileAlignment;
	//modify sizeofimage
	DWORD opt_offset = ((DWORD)optional_header - (DWORD)dos_header);
	IMAGE_OPTIONAL_HEADER* optheader = (IMAGE_OPTIONAL_HEADER*)((DWORD)imagebuffer + opt_offset);
	optheader->SizeOfImage = optional_header->SizeOfImage + 0x1000;
	memcpy(sheader[new_num_section - 1].Name, ".my", 9);
	return 0;
}
int enlarge_section(LPVOID filebuffer, VOID* enlarge_buffer)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((DWORD)filebuffer + dos_header->e_lfanew);
	IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
	IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
	DWORD num_sections = file_header->NumberOfSections; 


	DWORD section_offset = ((DWORD)section_header - (DWORD)dos_header);
	IMAGE_SECTION_HEADER* sheader = (IMAGE_SECTION_HEADER*)((DWORD)enlarge_buffer + section_offset);
	DWORD opt_offset = ((DWORD)optional_header - (DWORD)dos_header);
	IMAGE_OPTIONAL_HEADER* optheader = (IMAGE_OPTIONAL_HEADER*)((DWORD)enlarge_buffer + opt_offset);

	sheader[num_sections - 1].SizeOfRawData += 0x1000;
	sheader[num_sections - 1].Misc.VirtualSize += 0x1000;
	optheader->SizeOfImage = optional_header->SizeOfImage + 0x1000;
	return 0;
}
int merge_section(LPVOID filebuffer, VOID* imagebuffer)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((DWORD)filebuffer + dos_header->e_lfanew);
	IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
	IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
	DWORD num_sections = file_header->NumberOfSections;

	DWORD section_offset = ((DWORD)section_header - (DWORD)dos_header);
	IMAGE_SECTION_HEADER* sheader = (IMAGE_SECTION_HEADER*)((DWORD)imagebuffer + section_offset);
	DWORD opt_offset = ((DWORD)optional_header - (DWORD)dos_header);
	IMAGE_OPTIONAL_HEADER* optheader = (IMAGE_OPTIONAL_HEADER*)((DWORD)imagebuffer + opt_offset);
	DWORD file_offset = ((DWORD)file_header - (DWORD)dos_header);
	IMAGE_FILE_HEADER* fheader = (IMAGE_FILE_HEADER*)((DWORD)imagebuffer + file_offset);


	sheader[0].SizeOfRawData = optheader->SizeOfImage - sheader[0].VirtualAddress;
	sheader[0].Misc.VirtualSize = optheader->SizeOfImage - sheader[0].VirtualAddress;
	fheader->NumberOfSections = 1;
	sheader[0].Characteristics = 0xFF50DAE0; //打开所有权限
	memset(sheader + 1, 0, sizeof(IMAGE_SECTION_HEADER)*(num_sections-1)); // 清0
	return 0;
}
DWORD find_function_addressRVA_byname(LPVOID filebuffer, const char* name)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((DWORD)filebuffer + dos_header->e_lfanew);
	IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
	IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
	_IMAGE_EXPORT_DIRECTORY* extable = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)filebuffer + RVA_to_FOA(optional_header->DataDirectory[0].VirtualAddress, section_header, file_header));
	cout << "DLL's name : "<<(char*)((DWORD)filebuffer + RVA_to_FOA(extable->Name, section_header, file_header)) << endl; // Test dll's name
	int* name_address = (int*) ((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfNames, section_header, file_header)); //名字表，宽度4，双子就行
	short* ordinal_address = (short*)((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfNameOrdinals, section_header, file_header)); //序号表，单字节
	int* function_address = (int*)((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfFunctions, section_header, file_header));//函数表
	DWORD index = -1;
	for(int i = 0; i< extable->NumberOfNames;i++)
	{
		char* name_in_table = (char*)((DWORD)filebuffer + RVA_to_FOA(name_address[i], section_header, file_header));
		if(!strcmp(name, name_in_table))
		{
			index = i;
		}
	}
	if (index == -1 ) { cout << "no matching name" << endl; return 1; }
   //find address at the ordinal table
	DWORD ordinal_val = ordinal_address[index];
	if (ordinal_val >= extable->NumberOfFunctions) { cout << "no matching ordinal value" << endl; return 1;}
	cout << name<<"'s RVA is "<<hex << function_address[ordinal_val] << endl;
	return 0;
}
DWORD print_export_table(LPVOID filebuffer)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)filebuffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)((DWORD)filebuffer + dos_header->e_lfanew);
	IMAGE_FILE_HEADER* file_header = &nt_header->FileHeader;
	IMAGE_OPTIONAL_HEADER* optional_header = &nt_header->OptionalHeader;
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)((DWORD)optional_header + file_header->SizeOfOptionalHeader);
	_IMAGE_EXPORT_DIRECTORY* extable = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)filebuffer + RVA_to_FOA(optional_header->DataDirectory[0].VirtualAddress, section_header, file_header));
	cout << "DLL's name : " << (char*)((DWORD)filebuffer + RVA_to_FOA(extable->Name, section_header, file_header)) << endl; // Test dll's name
	int* name_address = (int*)((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfNames, section_header, file_header)); //名字表，宽度4，双子就行
	short* ordinal_address = (short*)((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfNameOrdinals, section_header, file_header)); //序号表，单字节
	int* function_address = (int*)((DWORD)filebuffer + RVA_to_FOA(extable->AddressOfFunctions, section_header, file_header));//函数表
	for (int i = 0; i < extable->NumberOfFunctions;i++)
	{
		for (int j = 0; j < extable->NumberOfNames; j++)
		{
			if (ordinal_address[j] == i)
			{
				cout << "Address Name: " << (char*)((DWORD)filebuffer + RVA_to_FOA(name_address[j], section_header, file_header)) << " Ordinal: " << i + extable->Base << " Function RVA: " << function_address[i] << endl;
			}
		}

	}
	return 0;
}
int main()
{
	char path[] = "C:\\PE\\advancedolly.dll";
	//filebuffer_to_imagebuffer_to_disk(path);
	LPVOID filebuffer;
	DWORD filesize = readfile(path, &filebuffer);
	if (!filesize)
	{
		cout << "failed to readfile at address: " << path << endl;
		return 0;
	}
	find_function_addressRVA_byname(filebuffer, "_ODBG_Plugindata");
	print_export_table(filebuffer);
	return 0;
}

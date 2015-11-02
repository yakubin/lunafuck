// Copyright: see LICENSE.txt
#include <elf.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "addrstack.h"
#include "codelist.h"
#include "hex.h"

#define ENTRY_POINT 0x08048000 + sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)

Elf32_Ehdr elf_header = {
    .e_ident          = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS32, ELFDATA2LSB,
                EV_CURRENT, ELFOSABI_SYSV, 0, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, EI_NIDENT},
    .e_type      = ET_EXEC,
    .e_machine   = EM_386,
    .e_version   = EV_CURRENT,
    .e_entry     = ENTRY_POINT,
    .e_phoff     = sizeof elf_header,
    .e_shoff     = 0,  // irrelevant
    .e_flags     = 0,  // irrelevant
    .e_ehsize    = sizeof elf_header,
    .e_phentsize = sizeof(Elf32_Phdr),
    .e_phnum     = 1,

    // irrelevant
    .e_shentsize = sizeof(Elf32_Shdr),
    .e_shnum     = 0,
    .e_shstrndx  = SHN_UNDEF};

Elf32_Phdr text_header = {
    .p_type            = PT_LOAD,
    .p_offset          = sizeof elf_header + sizeof text_header,
    .p_vaddr           = ENTRY_POINT,
    .p_paddr           = 0,
    .p_filesz          = 0xdeadbeaf,  // unknown at the time of compiling the compiler
    .p_memsz           = 0xdeadbeaf,   // == p_filesz
    .p_flags           = PF_R + PF_X,  // read+exec permissions
    .p_align           = 0x1000};

// safe version of writev(2) (makes sure all the bytes are written)
// returns false on failure; true on success
bool safe_writev(int fd, struct iovec* iov, int iovcnt) {
    size_t len = 0;
    for (int i = 0; i < iovcnt; ++i) {
        len += iov[i].iov_len;
    }

    ssize_t written_len;
    struct iovec* iovptr = iov;
    do {
        written_len = writev(fd, iovptr, iovcnt - (iovptr - iov));
        if (written_len == -1) {
            perror("couldn't write to output file");
            return false;
        }

        len -= written_len;

        for (ssize_t rel_len = 0; written_len > 0; ++iovptr) {
            rel_len = ((size_t)written_len > iovptr->iov_len)
                          ? written_len - iovptr->iov_len
                          : written_len;
            written_len -= rel_len;
            iovptr->iov_base += rel_len;
            iovptr->iov_len -= rel_len;
        }
    } while (len > 0);

    return true;
}

// converts code_list to elf and writes it to file of a certain path
//
// @param `path`:       path to the target file; MUST NOT BE NULL
// @param `code_list`:  the source code_list; MUST NOT BE NULL
//
// @return:
// true on success
// false on failure
bool write_elf(const char* path, struct code_list* code_list) {
    int fd = creat(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (fd == -1) {
        fprintf(stderr, "couldn't open file '%s': ", path);
        perror(NULL);
        return false;
    }

    uint8_t* text_seg = NULL;
    size_t text_len   = code_list_to_hex(&text_seg, code_list);

    text_header.p_filesz = text_header.p_memsz = text_len;

    struct iovec iov[] = {{&elf_header, sizeof elf_header},
                          {&text_header, sizeof text_header},
                          {text_seg, text_len}};

    if (safe_writev(fd, iov, 3) == false) {
        return false;
    }

    free(text_seg);
    return true;
}

// maps the contents of file pointeb by path to *dst
//
//
// @param `path`:   path to the file to map; MUST NOT BE NULL
// @param `dst`:    pointer to the place where to save to pointer to the
//                  mapping;
//                  * MUST NOT BE NULL
//                  * THE CALLER IS OBLIGED TO munmap(2) IT
//
// @return:
// -1   if couldn't open the file
// -2   if couldn't map the file
// size of the file on success
ssize_t load_file(const char* path, char** dst) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "couldn't open file '%s': ", path);
        perror(NULL);
        return -1;
    }

    struct stat st;
    fstat(fd, &st);

    char* src = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // safe, doesn't unmap the region
    if (src == MAP_FAILED) {
        fprintf(stderr, "couldn't load file '%s': ", path);
        perror(NULL);
        return -2;
    }

    *dst = src;
    return st.st_size;
}

void help() {
    fprintf(stderr,
            "lunafuck 0.1.0\n\n\tusage: lunafuck output_file input_file\n\n");
    fprintf(
        stderr,
        "Copyright (c) 2015, Jakub Kucharski <prooxod@gmail.com>\nAll rights "
        "reserved.\n\nRedistribution and use in source and binary forms, with "
        "or without modification,\nare permitted provided that the following "
        "conditions are met:\n\n1. Redistributions of source code must retain "
        "the above copyright notice, this\n   list of conditions and the "
        "following disclaimer.\n\n2. Redistributions in binary form must "
        "reproduce the above copyright notice,\n   this list of conditions and "
        "the following disclaimer in the documentation\n   and/or other "
        "materials provided with the distribution.\n\n3. Neither the name of "
        "the copyright holder nor the names of its contributors\n   may be "
        "used to endorse or promote products derived from this software "
        "without\n   specific prior written permission.\n\nTHIS SOFTWARE IS "
        "PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\nANY "
        "EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE "
        "IMPLIED\nWARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR "
        "PURPOSE ARE\nDISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR "
        "CONTRIBUTORS BE LIABLE\nFOR ANY DIRECT, INDIRECT, INCIDENTAL, "
        "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\nDAMAGES (INCLUDING, BUT NOT "
        "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\nSERVICES; LOSS OF "
        "USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\nCAUSED AND "
        "ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
        "LIABILITY,\nOR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN "
        "ANY WAY OUT OF THE USE\nOF THIS SOFTWARE, EVEN IF ADVISED OF THE "
        "POSSIBILITY OF SUCH DAMAGE.\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        help();
        return 1;
    }

    char* src      = NULL;
    ssize_t srclen = load_file(argv[2], &src);
    if (srclen < 0) {
        return 2;
    }

    struct code_list* code_list = make_code_list(src, srclen);
    if (code_list == NULL) {
        return 3;
    }

    bool status = write_elf(argv[1], code_list);
    munmap(src, srclen);
    free_code_list(code_list);
    if (status == false) {
        return 4;
    }

    return 0;
}

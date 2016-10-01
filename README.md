# lunafuck
## X86-32 brainfuck compiler for Linux

It directly generates an elf file (no assembly, no linking).

## Installation (from source code)

```
    $ git clone https://github.com/yakubin/lunafuck.git
    $ cd lunafuck
    $ make
    $ sudo cp ./lunafuck /usr/local/bin/
```


## Usage

```
    lunafuck input_file.b output_file
```

## Contributing

1. Check for open issues or open a fresh issue to start a discussion around 
   a feature idea or a bug.

2. Fork the lunafuck repository on Github to start making your changes.

3. Add yourself to CONTRIBUTORS.txt.

4. Reformat your code with ClangFormat with .clang-format which is in the top 
   directory of the repository. (ClangFormat >= 3.7)

5. Send me a pull request.

If you want to create a logo for lunafuck do as specified above and place the 
logo in file logo.svg.

# Copyright

Copyright (c) 2015-2016, Jakub Kucharski <jakubkucharski97@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this 
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
   may be used to endorse or promote products derived from this software without 
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED  
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE  
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE  
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL  
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR  
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER  
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,  
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

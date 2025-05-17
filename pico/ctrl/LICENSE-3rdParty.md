# Licenses and Acknowledgements

This contains sections with an acknowledgement and license information for
the 3rd-Party libraries and code used within the HWControl software.

## Raspberry Pi Pico SDK Examples

The Pico SDK is used to build the operating code. Various Pico SDK Examples
provided guidance for using the Pico and some of the operations. Code used for
the HWControl operating software was rarely copied from any of the examples. If
any significant portion was copied it has been noted in the source file where
it was used. That being said, any code used from the SDK or the SDK Examples is:

* Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The SDK is available here: https://github.com/raspberrypi/pico-examples

## JSON-Maker

JSON-Maker from rafagafe is used in this project and is located in the src/lib/json-maker directory.

It is modified slightly to adapt to the build directory structure, and only the module source and
README.md are included. The complete, unmodified, library can be obtained from the repository
here: https://github.com/rafagafe/json-maker

It is licensed under the MIT License included here:

MIT License

Copyright (c) 2018 Rafa Garcia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Tiny-JSON

Tiny-JSON from rafagafe is used in this project and is located in the src/lib/tiny-json directory.

It is modified slightly to adapt to the build directory structure, and only the module source and
README.md are included. The complete, unmodified, library can be obtained from the repository
here: https://github.com/rafagafe/tiny-json

It is licensed under the MIT License included here:

MIT License

Copyright (c) 2018 Rafa Garcia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Exceptions in C with Longjmp and Setjmp (Try-Throw-Catch-Finally)

Francesco Nidito described a simple implementation of a Try-Catch-Finally mechanism
that can be used in mainline code to handle errors within called functions without
having to return error codes from each level. The article can be read in its entirety here:
http://groups.di.unipi.it/~nids/docs/longjump_try_trow_catch.html

The implementation defined in the article is licensed under the MIT license include here:

Copyright (C) 2009-2014 Francesco Nidito

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### Modification

The implementation in `try_throw_catch.h` has been modified slightly to make it
compliant with the C standard by changing `ETRY` to `Etry`, as identifiers or macros
starting with capital E and followed by another capital letter or number are
reserved for use by the standard <errno.h> include file. The be consistent, all
the macros were changed to mixed case.

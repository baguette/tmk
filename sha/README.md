# sha1_main is a sha1 test driver program

SHA1 implementation that we will use in tmake

## Building and Testing

|                                          Command                             |                                    Result                                      |
|------------------------------------------------------------------------------|--------------------------------------------------------------------------------|
| cc -o sha1test sha1_main.c sha1.c                                            | sha1test program that can produce the sha1 sum. Tool takes 1 argument.         |
| ./sha1sum FILE                                                               | 6cb14a7e14e1e6262f468c7b99f11aae3bace7d8  FILE                                 |

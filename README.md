# In-Memory-Process-Memory-Dump
A proof of concept of capturing a processes memory, saving it in memory without writing to a file on the disk, then sending it over the network to a remote computer. The code requires administrator privileges to run and most likely antivirus has to be turned off since theres no evasion techiques used in this demo. If you run the server file on windows av will flag it also because to the computers its a process thats writing a random file to the disk. Recommended test use is to run the linux server code and have the main code connect to that to avoid any issues and enable more access to tools to dissect the memory dumps

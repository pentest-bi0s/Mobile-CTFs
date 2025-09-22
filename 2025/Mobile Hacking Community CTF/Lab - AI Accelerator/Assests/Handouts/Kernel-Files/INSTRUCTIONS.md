The challenge source code is located at ./kernel/drivers/char/ai/

A debug environment has been provided to develop your exploit.

Kernel hashes and config files for both remote and debug environment is also provided.

NOTE: selinux is NOT turned on in the debug kernel, but it is available in the remote corellium device

Debug environment creds
- mhl:hacker
- root:toor

Copy files into debug env using scp
- `scp -P10023 'exploit' 'mhl@localhost:/home/mhl/exploit'`

On remote flag file is located at /data/local/tmp/root/flag.txt

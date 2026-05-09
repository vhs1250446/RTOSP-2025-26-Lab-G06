#!/bin/bash
start=$(date +'%s')
# cd linux-6.19.9-moker
make 2>../errors-6.19.9-moker
sudo make modules_install compile_commands.json
sudo make install
cd ..
sudo update-grub2
cat errors-6.19.9-moker
echo "Linux kernel compilation and installation took $(($(date +'%s') - $start)) seconds"

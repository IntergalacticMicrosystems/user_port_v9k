#/bin/zsh
cd /Users/pauldevine/projects/openwatcomv2/
source ~/projects/openwatcomv2/mysetvars.sh
cd /Users/pauldevine/projects/user_port_v9k/victor9k/src
wmake clean all
vcopy /Users/pauldevine/Desktop/randos/userport.img /Users/pauldevine/projects/user_port_v9k/victor9k/src/userport.sys :

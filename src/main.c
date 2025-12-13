#include "Commands/copying.h"
#include "Commands/watcher.h"
#include "DataStructures/avl_map.h"
#include "stdio.h"


int main(int argc, char* argv[]){
    copy_info info;
    info.root_src = "F";
    info.root_dst = "G";
    controlPanel* panel = initialize_watch(&info);
    char str[100];
    scanf("%s", str);
    panel->watch = 0;
}
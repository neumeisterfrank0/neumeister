#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <sys/swap.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/acl.h>

int main(void) {
    sleep(1);

    // 1. Establish root session
    //
    setgid(0);
    setuid(0);
    setsid();
        
    // 2. Create Base Directories & Permissions
    //
    mkdir("/proc", 0555);
    mkdir("/sys" , 0755);
    mkdir("/dev" , 0755);
    mkdir("/mnt" , 0755);
    mkdir("/run" , 0755);
    mkdir("/tmp" , 1777);
    mkdir("/home", 0755);
    mkdir("/boot", 0755);

    // 3. Mount Base Filesystems
    //
    mount("proc"    , "/proc", "proc"    , MS_NOSUID | MS_NOEXEC | MS_NODEV  , NULL);
    mount("sysfs"   , "/sys" , "sysfs"   , MS_NOSUID | MS_NOEXEC | MS_NODEV  , NULL);
    mount("devtmpfs", "/dev" , "devtmpfs", MS_NOSUID                         , "mode=755");
    mount("tmpfs"   , "/mnt" , "tmpfs"   , MS_RELATIME                       , "mode=1777,size=256M");
    mount("tmpfs"   , "/run" , "tmpfs"   , MS_NOSUID | MS_RELATIME | MS_NODEV, "mode=0755,size=256M");
    mount("tmpfs"   , "/tmp" , "tmpfs"   , MS_NOSUID | MS_NODEV              , "mode=777,size=512M");
    

    // 4. Setup Dev Sub-directories & Sub-mounts
    //
    mkdir("/dev/pts", 0755);
    mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC, "gid=5,mode=620,ptmxmode=666");
    mkdir("/dev/shm", 1777);
    mount("tmpfs", "/dev/shm" , "tmpfs" , MS_NOSUID | MS_NODEV , "mode=1777,size=256M");
	
    // 5. Create Base Symlinks
    //
    symlink("/proc/self/fd", "/dev/fd");
    
    // 6. Mount root partition
    //
    mount("/dev/sda3", "/mnt" , "ext4", MS_RELATIME, NULL);
    mount("/dev/sda5", "/home", "ext4", MS_RELATIME, NULL);
    mount("/dev/sda2", "/boot", "ext4", MS_RELATIME, NULL);
    
    // 7. Prepare Target Transition (Pivot Root Setup)
    //
    mkdir("/mnt/old_root", 0755);
    mkdir("/mnt/proc"    , 0555);
    mkdir("/mnt/sys"     , 0755);
    mkdir("/mnt/dev"     , 0755);
    mkdir("/mnt/run"     , 0755);
    mkdir("/mnt/tmp"     , 1777);
    mkdir("/mnt/home"    , 0755);
    mkdir("/mnt/boot"    , 0755);
    mount("/proc", "/mnt/proc", NULL, MS_MOVE, NULL);
    mount("/sys" , "/mnt/sys" , NULL, MS_MOVE, NULL);
    mount("/dev" , "/mnt/dev" , NULL, MS_MOVE, NULL);
    mount("/run" , "/mnt/run" , NULL, MS_MOVE, NULL);
    mount("/tmp" , "/mnt/tmp" , NULL, MS_MOVE, NULL);
    mount("/home", "/mnt/home", NULL, MS_MOVE, NULL);
    mount("/boot", "/mnt/boot", NULL, MS_MOVE, NULL);
    
    
    // 8. Execute Pivot Root
    // 
    chdir("/mnt");
    syscall(SYS_pivot_root, ".", "old_root");
    chdir("/");

    // 9. Unmount and remove Old Root
    //
    umount2("/old_root", MNT_DETACH);
    rmdir("/old_root");
    
    // 10. Create System Symlinks
    //
    symlink("/usr/lib", "/lib64");
    symlink("/usr/lib", "/lib");
    symlink("/usr/bin", "/bin");
    
    // 15. System Clock (RTC) - Using BIOS time as-is
    // 
    printf("System clock initialized to hardware BIOS time.\n");
    
    // 16. Use internal engine forks to clear dynamic files safely without running system() shell calls
    //
    //system("rm -rf /run/*");
    //system("rm -rf /tmp/*");
    mkdir("/run/dbus", 0755);
    mkdir("/run/user", 0755);
    mkdir("/run/user/0", 0755);
    
    // 17. Root Environment & Controlling Terminal Setup
    //
    int fd = open("/dev/tty0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/tty0\n");
        sleep(10);
        return 1;
    } else {
        printf("/dev/tty0 opened successfully\n");
        if (ioctl(fd, TIOCSCTTY, 1) < 0) {
            perror("ioctl TIOCSCTTY failed\n");
            close(fd);
            return 1;
        } else {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2) {
                close(fd);
            }
            printf("PID 1 i/o control claimed\n");
        }
    } 
    
    // 17.  Get the make model and serial number of the computer for use as hostname
    //
    char make[64];
    char model[64];
    char serial[64];
    FILE *make_file;
    FILE *model_file;
    FILE *serial_file;
    make_file = fopen("/sys/class/dmi/id/sys_vendor", "r");
    model_file = fopen("/sys/class/dmi/id/product_name", "r");
    serial_file = fopen("/sys/class/dmi/id/product_serial", "r");
    fgets(make, sizeof(make), make_file);
    fgets(model, sizeof(model), model_file);
    fgets(serial, sizeof(serial), serial_file);
    make[strcspn(make, "\n")] = '\0';
    model[strcspn(model, "\n")] = '\0';
    serial[strcspn(serial, "\n")] = '\0';
    fclose(make_file);
    fclose(model_file);
    fclose(serial_file);
    char hostname[256];
    //snprintf(hostname, sizeof(hostname), "%s-%s-%s", make, model, serial);
    snprintf(hostname, sizeof(hostname), "%s-%s", make, serial);
    sethostname(hostname,sizeof(hostname));
    setenv("HOSTNAME",hostname,1);
    FILE *hostname_file=fopen("/etc/hostname","w");
    fprintf(hostname_file,"%s\n",hostname);
    char environment_hostname[256];
    snprintf(environment_hostname, sizeof(environment_hostname), "HOSTNAME=%s", hostname);
    
    // 18. Establish Root environment variables
    //
    char *root_environment[] = {
        "PATH=/bin:/usr/bin",
        "LIBRARY_PATH=/lib:/usr/lib",
        "LD_LIBRARY_PATH=/lib:/usr/lib",
        "HOME=/root",
        "XAUTHORITY=/root/.Xauthority",
        "USER=root",
        "LOGNAME=root",
        "SHELL=/usr/bin/bash",
        "TERM=xterm-256color",
        "LANG=en_US.UTF-8",
        "LC_ALL=en_US.UTF-8",
        "DISPLAY=:0",
        "XDG_SESSION_TYPE=x11",
        "XDG_RUNTIME_DIR=/run/user/0",
        "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/dbus",
        environment_hostname,
	NULL
    };
    
    // 19, Set root environment variables in system
    //
    for (int i = 0; root_environment[i] != NULL; i++) {
        char *env_copy = strdup(root_environment[i]);
        if(env_copy!=NULL){
	    char *equals_sign = strchr(env_copy, '=');
            if(equals_sign==NULL){
	        free(env_copy);
	    }
	    *equals_sign = '\0'; 
	    char *key = env_copy;
            char *value = equals_sign + 1;
            setenv(key, value, 1); 
            free(env_copy);
        }
    }
    
    // 12. Setup Swap space using ZRAM
    //
    struct sysinfo info;
    if(sysinfo(&info) == 0){
    	unsigned long long total_ram_bytes =(unsigned long long)info.totalram * info.mem_unit;
    	unsigned long long target_swap_bytes = 0;
    	unsigned long long non_swap_bytes = 256ULL * 1024ULL * 1024ULL;
        target_swap_bytes = total_ram_bytes - non_swap_bytes; // Minimal fallback limit allocation
    	FILE *f = fopen("/sys/block/zram0/disksize", "w");
    	fprintf(f, "%llu", target_swap_bytes);
    	fclose(f);
        system("/usr/bin/mkswap /dev/zram0");
        system("/usr/bin/swapon /dev/zram0");
    }
    // 13. Enable Hardware Drive Swap Partition
    // 
    int swap_flags = SWAP_FLAG_PREFER | ((101 << SWAP_FLAG_PRIO_SHIFT) & SWAP_FLAG_PRIO_MASK);
    swapon("/dev/sda4", swap_flags);
    
    
    // launch system program binaries
    //     
    pid_t udevd_pid=fork();
    if(udevd_pid==0){
    	char *udevd_array[] = {"systemd-udevd", "--daemon", NULL};
    	printf("systemd-udevd --daemon\n");
	execve("/usr/lib/systemd/systemd-udevd", udevd_array, root_environment);
    }else{
    	waitpid(udevd_pid,NULL,0);
    }

    pid_t udevadm_control_pid=fork();
    if(udevadm_control_pid==0){
    	char *udevadm_control_array[] = {"udevadm", "control", "--reload-rules", NULL};
    	printf("udevadm control --reload-rules\n");
	execve("/usr/bin/udevadm", udevadm_control_array, root_environment);
    }else{
	waitpid(udevadm_control_pid,NULL,0);
    }
    
    pid_t udevadm_trigger_pid=fork();
    if(udevadm_trigger_pid==0){
    	char *udevadm_trigger_array[] = {"udevadm", "trigger", NULL};
    	printf("udevadm trigger\n");
    	execve("/usr/bin/udevadm", udevadm_trigger_array, root_environment);
    }else{
	waitpid(udevadm_trigger_pid,NULL,0);
    }
    
    pid_t udevadm_settle_pid=fork();
    if(udevadm_settle_pid==0){
    	char *udevadm_settle_array[] = {"udevadm", "settle",  NULL};
    	printf("udevadm settle\n");
    	execve("/usr/bin/udevadm", udevadm_settle_array, root_environment);
    }else{
	waitpid(udevadm_settle_pid,NULL,0);
    }
    
    pid_t dbus_system_pid=fork();
    if(dbus_system_pid==0){
    	char *dbus_system_array[] = {"dbus-daemon", "--system",  NULL};
    	printf("dbus-daemon --system\n");
	execve("/usr/bin/dbus-daemon", dbus_system_array, root_environment);
    }else{
	waitpid(dbus_system_pid,NULL,0);
    }

    if(fork()==0){
	char *NetworkManager_array[] = {"NetworkManager", NULL};
	printf("NetworkManager\n");
    	execve("/usr/bin/NetworkManager", NetworkManager_array, root_environment);
    }else{
        sleep(1);
    }
	
    if(fork()==0){
        char *bluetoothd_array[] = {"bluetoothd", NULL};
	printf("bluetoothd\n");
        execve("/usr/lib/bluetooth/bluetoothd", bluetoothd_array, root_environment);
    }else{
        sleep(1);
    }

    if(fork()==0){
    	char *crond_array[] = {"crond", "-Pp", NULL};
	printf("crond -Pp\n");
    	execve("/usr/bin/crond", crond_array, root_environment);
    }else{
        sleep(1);	    
    }

    if(fork()==0){
    	char *avahi_array[] = {"/usr/bin/avahi-daemon", NULL};
    	printf("avahi-daemon\n");
	execve("/usr/bin/avahi-daemon",avahi_array, root_environment);
    }else{
        sleep(1);
    }

    // 19. fork PID #1 to reap zombie proccesses
    //
    if(fork()!=0){
        while(1){
	    sleep(2);
            waitpid(-1, NULL, WNOHANG);
	}
    }

    // set system devices to user ownership
    //
    system("/usr/bin/setfacl -m u:user:rw /dev/snd/*");
    system("/usr/bin/setfacl -m u:user:rw /dev/video*");
    system("/usr/bin/setfacl -m u:user:rw /dev/media*");
    system("/usr/bin/setfacl -m u:user:rw /dev/dri/*");
    system("/usr/bin/setfacl -m u:user:rw /dev/input/*");
    system("/usr/bin/setfacl -m u:user:rw /dev/uinput*");
    system("/usr/bin/setfacl -m u:user:rw /dev/dma_heap/*");
    system("/usr/bin/setfacl -m u:user:rw /dev/sr*");

    // prepare usre runtime directories
    //
    mkdir("/run/user", 0755);
    mkdir("/run/user/1000", 0755);
    mkdir("/run/user/1000/dbus", 0755);
    chown("/run/user/1000", 1000, 1000);
    chdir("/home/user");
    
    // set user environental variables
    // 
    char *user_environment[] = {
        "PATH=/bin:/usr/bin",
        "LIBRARY_PATH=/lib:/usr/lib",
        "LD_LIBRARY_PATH=/lib:/usr/lib",
        "HOME=/home/user",
        "USER=user",
        "LOGNAME=user",
        "SHELL=/usr/bin/bash",
        "TERM=xterm-256color",
        "LANG=en_US.UTF-8",
        "LC_ALL=en_US.UTF-8",
        "HOSTNAME=localhost",
        "PWD=/home/user",
        "XDG_RUNTIME_DIR=/run/user/1000",
        "XDG_SESSION_TYPE=x11",
        "XDG_CURRENT_DESKTOP=Openbox",
        "XDG_CONFIG_HOME=/home/user/.config",
        "XDG_CACHE_HOME=/home/user/.cache",
        "XDG_DATA_HOME=/home/user/.local/share",
        "DISPLAY=:0",
        "XAUTHORITY=/home/user/.Xauthority",
        "COLORTERM=truecolor",
        "GTK_IM_MODULE=ibus",
        "QT_IM_MODULE=ibus",
        "XMODIFIERS=@im=ibus",
        "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/dbus",
        environment_hostname,
        NULL
    };
    
    // Apply Enviromental variables for user 
    //
    for (int i = 0; user_environment[i] != NULL; i++) {
        char *env_copy = strdup(user_environment[i]);
        if(env_copy!=NULL){
	    char *equals_sign = strchr(env_copy, '=');
            if(equals_sign==NULL){
	        free(env_copy);
	    }
	    *equals_sign = '\0'; 
	    char *key = env_copy;
            char *value = equals_sign + 1;
            setenv(key, value, 1); 
            free(env_copy);
        }
    }
    
    // start user session
    //
    setgid(1000);
    setuid(1000);
    setsid();

	
    // launch user space desktop applicationa
    //
    if(fork()==0){	
        char *dbus_session_array[] = {"dbus-daemon", "--session", NULL};
        printf("dbus-daemon --session\n");
	execve("/usr/bin/dbus-daemon", dbus_session_array, user_environment);
    }else{
        sleep(1);
    }
    
    if(fork()==0){
        char *Xorg_array[] = {"Xorg", ":0", NULL};
        printf("Xorg :0\n");
	execve("/usr/bin/Xorg", Xorg_array, user_environment);
    }else{
        sleep(3);
    }	
    
    if(fork()==0){	
        char *pipewire_array[] = {"pipewire", NULL};
        printf("pipewire\n");
	execve("/usr/bin/pipewire", pipewire_array, user_environment);
    }else{
        sleep(1);
    }

    if(fork()==0){
        char *pipewire_pulse_array[] = {"pipewire-pulse", NULL};
        printf("pipewire-pulse\n");
	execve("/usr/bin/pipewire-pulse", pipewire_pulse_array, user_environment);
    }else{
        sleep(1);
    }

    if(fork()==0){
        char *wireplumber_array[] = {"wireplumber", NULL};
        printf("wireplumber\n");
	execve("/usr/bin/wireplumber", wireplumber_array, user_environment);
    }else{
        sleep(1);
    }

    if(fork()==0){
        char *feh_array[] = {"feh", "--randomize", "--bg-max", "/home/user/Pictures/Wallpapers", NULL};
        printf("feh --randomize --bg-max /home/user/Pictures/Wallpapers\n");
	execve("/usr/bin/feh", feh_array, user_environment);
    }
    
    if(fork()==0){
        char *tint2_array[] = {"tint2", NULL};
        printf("tint2\n");
	execve("/usr/bin/tint2", tint2_array, user_environment);
    }
    
    if(fork()==0){
	char *nm_applet_array[] = {"dbus-run-session", "--", "nm-applet", NULL};
    	printf("dbus-run-session -- nm-applet");
	execve("/usr/bin/dbus-run-session", nm_applet_array, user_environment);
    }
    
    if(fork()==0){
        char *blueman_applet_array[] = {"dbus-run-session", "--", "blueman-applet", NULL};
	printf("dbus-run-session -- blueman-applet");
	execve("/usr/bin/dbus-run-session", blueman_applet_array, user_environment);
    }

    if(fork()==0){
        char *xscreensaver_array[] = {"xscreensaver", "--no-splash", NULL};
        printf("xscreensaver --no-splash");
	execve("/usr/bin/xscreensaver", xscreensaver_array, user_environment);
    }
    
    if(fork()==0){
        char *flameshot_array[] = {"/usr/bin/dbus-run-session", "--", "/usr/bin/flameshot", NULL};
    	printf("dbus-run-session -- flameshot");
	execve("/usr/bin/dbus-run-session", flameshot_array, user_environment);
    }
    
    if(fork()==0){
        char *ibus_array[] = {"/usr/bin/ibus-daemon", "-xd", NULL};
        printf("ibus-daemon -xd");
	execve("/usr/bin/ibus-daemon",ibus_array, user_environment);
    }
    
    pid_t mpv_pid=fork();
    if(mpv_pid==0){
        char *mpv_array[] = {"/usr/bin/mpv", "/home/user/Music/Windows_95_Startup-Microsoft.wav", NULL};
    	printf("mpv /home/user/Music/Windows_95_Startup-Microsoft.wav");
	execve("/usr/bin/mpv",mpv_array, user_environment);
    }else{
        waitpid(mpv_pid,NULL,0);
    }
    
    if(fork()==0){
        char *openbox_array[] = {"openbox", NULL};
        printf("openbox\n");
	execve("/usr/bin/openbox",openbox_array, user_environment);
    }


    // Continuous local session loop keeping the graphics pipeline running smoothly
    while (1) {
        sleep(2);
        waitpid(-1, NULL, WNOHANG);
    }
}

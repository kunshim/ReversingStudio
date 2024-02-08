import psutil
import os
import subprocess
import tempfile
import signal
import sys
import platform
import ctypes

version = '1.0.0'

class Colors:
    RESET = "\033[0m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"

def signal_handler(sig, frame):
    global pid
    os.kill(pid, 9)
    print('\n[+] Bye~')
    sys.exit(0)

def get_pid_by_process_name(process_name):
    pid = None
    for proc in psutil.process_iter(['pid', 'name']):
        if process_name in proc.info['name']:
            pid = proc.info['pid']
            break
    return pid

def is_pid_64bit(pid):
    try:
        process_handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, pid)
        if process_handle:
            wow64 = ctypes.c_int()
            ctypes.windll.kernel32.IsWow64Process(process_handle, ctypes.byref(wow64))
            return wow64.value == 0  # 0이면 64-bit, 1이면 32-bit
    except Exception as e:
        print(f"Error: {e}")
        return False

def print_option_unique():
    print

if (len(sys.argv) >= 2 and sys.argv[1] == '--help') or len(sys.argv) == 1:
    print(f"{Colors.GREEN}Reversing Studio{Colors.RESET} {version}")
    print("Osori(@OSORI) <kunshim@naver.com>")
    print("Windows reversing tool based on intel pin")
    print(f'{Colors.CYAN}This script is only a wrapper for communicating with pintool and executing pin{Colors.RESET}')
    print("Usage : python3 pinpy.py [PROCESS NAME/PID/PATH] [DLL] [OPTIONS]")
    print(f'{Colors.YELLOW}DLL{Colors.RESET}')
    print(f"{'wndproc <options> ':<45} {'Pintool for find windows procedures':<30}")
    print(f"{'unique <target1> <target2> ... <options>':<45} {'Pintool for find unique call tracks':<30}")
    print(f"{'taint <target> <constraint> <options>':<45} {'Data tracker for specific functions return or argument':<30}")
    print(f'{Colors.YELLOW}Options for wndproc{Colors.RESET}')
    print(f"{'arg<0~3>=<value> (Default:arg1=0x111)': <45} {'Set wndproc argument value':<30}")    
    print(f'{Colors.YELLOW}Options for unique{Colors.RESET}')
    print(f"{'unique(u)=true/false (Default:false)': <45} {'Save all call tracks if not unique ':<30}")
    print(f"{'cov(c)=true/false (Default:false)': <45} {'Save coverage file for lighthouse':<30}")
    print(f"{'indirect(i)=true/false (Default:false)': <45} {'Save indirect call file for IDA plugin ':<30}")
    print(f"{'display-branch(db)=true/false (Default:true)': <45} {'Print branch information ':<30}")
    print(f"{'track-system(ts)=true/false (Default:false)': <45} {'Tracking inside system call (Low performance) ':<30}")
    print(f'{Colors.YELLOW}Options for taint{Colors.RESET}')
    print(f"{'show-all(sa) (Default:false)': <45} {'Print all data-relative instruction':<30}")
    print()
    exit(0)
signal.signal(signal.SIGINT, signal_handler)
pid = get_pid_by_process_name(sys.argv[1])
print('[+] Pid', pid)
if pid == None:
    print('[!] Cannot find process')
    exit(-1)
#Write your pinpath here!
pin_path = "E:/ReversingStudio/Pin/pin.exe"

default = {}
default['track-system'] = 'false'
default['indirect'] = 'false'
default['cov'] = 'false'
default['display-branch'] = 'true'
default['cmd'] = str(0x111)

input_file = open(tempfile.gettempdir() + "\\rsinput.txt", 'w')
for i in range(3, len(sys.argv)):
    t = sys.argv[i].split('=')
    if len(t) >= 2 and t[0] in default:
        default[t[0]] = t[1]
        continue
    input_file.write(sys.argv[i] + '\n')
for key in default:
    input_file.write(key + '=' + default[key] + '\n')
input_file.close()
print([pin_path, '-pid', str(pid), '-t', 'obj-x86/' + sys.argv[2]])
if not is_pid_64bit(pid):
    subprocess.run([pin_path, '-pid', str(pid), '-t', 'obj-x86/' + sys.argv[2]] , shell=True)
else:
    subprocess.run([pin_path, '-pid', str(pid), '-t', 'obj-x64/' + sys.argv[2]] , shell=True)
import psutil
import os
import subprocess
import sys

def get_pid_by_process_name(process_name):
    pid = None
    for proc in psutil.process_iter(['pid', 'name']):
        if process_name in proc.info['name']:
            pid = proc.info['pid']
            break
    return pid

pid = get_pid_by_process_name(sys.argv[1])
print('[+] Pid', pid)
pin_path = "E:/ReversingStudio/Pin/pin.exe"
print([pin_path, '-pid', str(pid), '-t', 'obj-x86/' + sys.argv[2]])
subprocess.run([pin_path, '-pid', str(pid), '-t', 'obj-x86/' + sys.argv[2]] , shell=True)
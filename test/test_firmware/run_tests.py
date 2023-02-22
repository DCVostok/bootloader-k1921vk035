import serial
import os
Import("env")
platform = env.PioPlatform()

def test_firmware_alive(target, source, env):
    TEST_READ_STR = "led state"
    print(env.GetProjectOption("monitor_port"))
    print(env.GetProjectOption("monitor_speed"))
    ser = serial.Serial(env.GetProjectOption("monitor_port"),env.GetProjectOption("monitor_speed"), timeout=2)
    print("Try read from device:")
    read_bytes = ser.read_until(expected=TEST_READ_STR.encode("ascii"), size=None)
    print(read_bytes)
    read_str = read_bytes.decode("ascii")
    print(read_str)
    if read_str.find(TEST_READ_STR) < 0:
        return -1
    else:
        return 0
    
flasher = os.path.join(platform.get_package_dir("tool-k1921vkx-flasher"),"k1921vkx_flasher.py") 
flasher_flags_test_read= ["-cr ","-f","mflash","-n", "main","-F 0","-p","$UPLOAD_PORT","-b 460800","--file","firmware_read.bin"]
flasher_flags_test_earse= ["-ce ","-f","mflash","-n", "main","-F 0","-p","$UPLOAD_PORT","-b 460800"]
flasher_flags_test_set_cfgword= ["-c ","-p","$UPLOAD_PORT","-b 460800","--set_cfgword FLASHRE=1,NVRRE=1,JTAGEN=1,DEBUGEN=1,NVRWE=1,FLASHWE=1,BMODEDIS=0"]

env.AddCustomTarget(
    name="run_tests",
    dependencies=["upload"],
    actions=[
        test_firmware_alive,
        "$PYTHONEXE %s %s"%(flasher," ".join(flasher_flags_test_read)),
        "$PYTHONEXE %s %s"%(flasher," ".join(flasher_flags_test_earse)),
        "$PYTHONEXE %s %s"%(flasher," ".join(flasher_flags_test_set_cfgword)),

    ],
    title="run_tests",
    description="Testing work together with flasher tool `k1921vkx_flasher`"
)


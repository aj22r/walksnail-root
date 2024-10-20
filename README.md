# Walksnail Avatar Goggles X rooting utility

This tool will allow you to change the root password on Walksnail Avatar Goggles X enabling you to gain access to your goggles through the pre-packaged SSH server.

## Usage

Set up a venv (optional, recommended)
```bash
python3 -m venv .venv
# Linux
. .venv/bin/activate
# Windows
.\.venv\Scripts\Activate.ps1
```

Install dependencies
```bash
pip3 install -r requirements.txt
```

Connect to your goggles (default wifi password is 12345678)

Run the exploit
```
python3 exploit.py
```
This will scan the entire DRAM range (1GiB), which may take several minutes and isn't always reliable.

If the exploit succeeds, you will be prompted to enter a new password for the root user, after which you can use `ssh root@192.168.42.1 -o HostKeyAlgorithms=+ssh-rsa` to access your goggles.

If the exploit fails, just power cycle your goggles and try again.

## What can I do with rooted goggles?

- Change the wifi password (recommended) - `python3 wificonf.py`
- Play Doom - `python3 doom.py`. Joystick button - FIRE, back button - USE, DVR button - ENTER. Make sure the display isn't turned off due to the proximity sensor when launching Doom.
- Play Bad Apple through the internal piezo buzzer - `python3 badapple.py`
- Reverse engineer stuff
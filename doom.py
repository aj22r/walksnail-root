import paramiko
import getpass
import time

host = "192.168.42.1"
username = "root"
password = getpass.getpass("Root password: ")

def transfer_file(client, local, remote):
    with open(local, "rb") as f:
        transport = client.get_transport()
        with transport.open_channel(kind="session") as channel:
            channel.exec_command(f"cat > {remote}")
            channel.sendall(f.read())

client = paramiko.client.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
client.connect(host, username=username, password=password)

print("Loading Doom")
transfer_file(client, "binaries/fbdoom", "/tmp/fbdoom")
transfer_file(client, "binaries/doom1.wad", "/tmp/doom1.wad")

print("Starting Doom. Ctrl-C to exit")

client.exec_command("kill `pidof fpv_grd_service`")

client.exec_command("chmod +x /tmp/fbdoom")
stdin, stdout, stderr = client.exec_command("/tmp/fbdoom -iwad /tmp/doom1.wad", get_pty=True)
try:
    # stdout.read().decode()
    while not stdout.channel.exit_status_ready():
        # print(stdout.readline())
        time.sleep(1)
except KeyboardInterrupt:
    stdout.channel.send("\x03")
else:
    print(stdout.read().decode())
    print(stderr.read().decode())

client.exec_command("LD_LIBRARY_PATH=/lib:/usr/lib:/local/lib:/local/usr/lib: /sbin/start-stop-daemon -S -x /local/usr/bin/fpv_grd_service 3")

client.close()
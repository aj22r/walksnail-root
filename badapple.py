import paramiko
import getpass

host = "192.168.42.1"
username = "root"
password = getpass.getpass("Root password: ")

client = paramiko.client.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
client.connect(host, username=username, password=password)

with open("binaries/badapple", "rb") as f:
    transport = client.get_transport()
    with transport.open_channel(kind='session') as channel:
        channel.exec_command('cat > /tmp/badapple')
        channel.sendall(f.read())

print("Ctrl-C to exit")

_, stdout, _ = client.exec_command("chmod +x /tmp/badapple")
stdin, stdout, _ = client.exec_command("/tmp/badapple", get_pty=True)
try:
    stdout.read().decode()
except KeyboardInterrupt:
    stdout.channel.send("\x03")

client.close()
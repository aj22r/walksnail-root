import paramiko
import getpass

host = "192.168.42.1"
username = "root"
password = getpass.getpass("Root password: ")

client = paramiko.client.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
client.connect(host, username=username, password=password)

wifi_pwd = getpass.getpass("New wifi password: ")
client.exec_command(f"sed -i '3c wifi_pwd={wifi_pwd}' /factory/user_wifi.ini")
client.exec_command("sync")

client.close()
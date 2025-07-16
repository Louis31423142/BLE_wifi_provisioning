import asyncio
import sys
import argparse

from bleak import BleakClient

ADDRESS = "2C:CF:67:BE:04:21"

SSID_CHARACTERISTIC = "b1829813-e8ec-4621-b9b5-6c1be43fe223"
PASSWORD_CHARACTERISTIC = "410f5077-9e81-4f3b-b888-bf435174fa58"

parser=argparse.ArgumentParser(description="ssid and password parser")
parser.add_argument("ssid")
parser.add_argument("password")
args = parser.parse_args()

ssid = args.ssid
password = args.password

async def main(ssid, password):
    async with BleakClient(ADDRESS) as client:
        print(f"Connected: {client.is_connected}")

        print("Writing SSID...")
        await client.write_gatt_char(SSID_CHARACTERISTIC, ssid.encode("utf-8"), response=True)
        await asyncio.sleep(1.0)
        print("Writing password...")
        await client.write_gatt_char(PASSWORD_CHARACTERISTIC, password.encode("utf-8"), response=True)
        await asyncio.sleep(1.0)

if __name__ == "__main__":
    asyncio.run(main(ssid, password))
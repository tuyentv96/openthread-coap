buildapp:
	west build -p -b nrf52840_pca10056 -- -DCONF_FILE="prj.conf overlay-ot.conf"
dongle:
	west build -p -b nrf52840_pca10059 -- -DCONF_FILE="prj.conf overlay-ot.conf"
flash:
	west flash
clean:
	rm -rf build
pkg:
	nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
        --application build/zephyr/zephyr.hex \
        --application-version 1 blinky.zip
boot:
	nrfutil dfu usb-serial -pkg blinky.zip -p /dev/ttyACM0
buildapp:
	west build -p -b nrf52840_pca10056 -- -DCONF_FILE="prj.conf overlay-ot.conf"
flash:
	west flash
clean:
	rm -rf build
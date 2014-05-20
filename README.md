dellfan - user space utility to control the fan speed on Dell Laptops.
======================================================================

* Allows to set the fan speed of a Dell laptop to 0 (disabled), 1 (medium) and 2 (full speed).


Caveats
-------

* The BIOS of some newer Dell laptops (Latitude E6420/E6520/E6430/E6530/E6440/E6540/...)
will override the speed you set unless you disable the BIOS control.
    * I found two methods of disabling the BIOS control:
        * By default ``DISABLE_BIOS_METHOD1`` will be used.
        * If you want to try ``DISABLE_BIOS_METHOD2`` edit the source code.
    * On my laptop (E6420), when you disable the BIOS control over the fan some weird things happen:
        * The Fn keys stop working.
        * Suspend to RAM sometimes stops working: The laptop will power itself off instead of suspending.
* Please, if you find a new method of disabling the BIOS control, let me know it.
    * There is a function that allows to discover new codes by proving (use with care).

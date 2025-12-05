.. include:: global.rst.inc
.. _setup_lora:

Setup (LoRA)
------------

The MultiGeiger can be connected with the followng steps to TTN_V3 (“The
Things Network”):

-  Create the TTN device in your profile at *The Things Network*
-  Transfer the parameters to the MultiGeiger
-  Login at *sensor.community*
-  Webhook integration

Creating a TTN device
~~~~~~~~~~~~~~~~~~~~~

The device must be registered with TTN. To do this, an account must
first be created at TTN (if one does not already exist).

Create TTN account
^^^^^^^^^^^^^^^^^^

At https://account.thethingsnetwork.org/register you must enter a
**USERNAME**, the **EMAIL ADDRESS** and a **PASSWORD**. Then click on
**Create account**.  After that you can log in to
the console with the new data at
https://account.thethingsnetwork.org/users/login.

Create application
^^^^^^^^^^^^^^^^^^

After logging in click on your name (upper right corner) and select **Console**.
On the next screen select **Europe 1** and click on **Continue as <your name>**. Then select **Applications** from the top menue, 
after that click on **+ Add application** (right blue button).

The following fields must be
filled in:

**Owner:**
  This is prefilled with your login name and need not to be changed.
**Application ID:**
   Any name for this application, but it must not yet exist in the
   network (e.g.: geiger_20220105).
**Application name**  
  Choose an arbitrary name for your application.
**Description:**
   Any description of the application can be entered here.

Now create the application by clicking the **Create application** button.

Create device
^^^^^^^^^^^^^

On the next page you can now create a new device. Press the button **+ Add end device**.
Now select **Manually** from the menu directly under **Register end device**.
Then the following fields must be filled in:

**Frequency plan**
  Select **Europe 863-870 MHz (SF9) for RX2 - recommended)**.
**LoRaWAN version**
  Select **MAC V1.0.3** (or V1.0.2).
**Regional Parameters version**
  Select **RP002-1.0.3** (or PHY V1.0.3 REV A).

**Activation mode**
  Select **Activation by personalization (ABP)**.

  .. note::
     MultiGeiger uses **ABP** instead of OTAA to ensure compatibility with single-channel
     LoRaWAN gateways (e.g., Dragino LG01-N) which cannot reliably handle OTAA join procedures.

After selecting ABP, three fields will appear. Click **Generate** for each:

**Device address (DevAddr)**
  Click **Generate** - a 4-byte address will be created (e.g., ``26011D01``).
**AppSKey (Application Session Key)**
  Click **Generate** - a 16-byte key will be created.
**NwkSKey (Network Session Key)**
  Click **Generate** - a 16-byte key will be created.

.. important::
   Write down these three values (DevAddr, AppSKey, NwkSKey). You will need them to configure
   your MultiGeiger device!

**Frame counter width**
  Leave at default (32 bit).

The last field (**End device ID**) will be filled in automatically.
Now press **Register end device**.

Enter the ABP parameters into the MultiGeiger
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After the registration was completed, the ABP parameters need to be entered into the MultiGeiger
configuration page.

1. Connect to your MultiGeiger's WiFi access point or open its IP address in your browser.
2. Navigate to the configuration page.
3. Scroll to the **LoRa Settings** section.
4. Enter the 3 ABP parameters from the TTN console (**DevAddr**, **NwkSKey**, **AppSKey**).

.. important::
   Enter the HEX values **without** spaces, exactly as they appear in the TTN console.

**Example:**

The TTN console shows:

::

   Device address:  26 01 1D 01
   NwkSKey:        1A 2B 3C 4D 5E 6F 7A 8B 9C AD BE CF D0 E1 F2 03
   AppSKey:        5E 6F 7A 8B 9C AD BE CF D0 E1 F2 03 14 25 36 47

Enter them in the MultiGeiger config page as:

::

   DevAddr (ABP):  26011D01
   NwkSKey (ABP):  1A2B3C4D5E6F7A8B9CADBECFD0E1F203
   AppSKey (ABP):  5E6F7A8B9CADBECFD0E1F203142536 47

.. tip::
   You can copy-paste the values from TTN, then remove all spaces using a text editor
   (Find & Replace: " " → "").

After entering these parameters:

5. Check the box **Send to LoRa (=>TTN)** to enable LoRa transmission.
6. Click **Save** at the bottom of the page.
7. **Restart** the MultiGeiger for the changes to take effect.

Logging data to sensor.community
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
If you want to transfer the data from Multigeiger to sensor.community, you have to 
register at sensor.community. The registration process is similar to the description 
above ("Login to sensor.community").  In the following, only the changes are
explained:

**Sensor ID:**
   Enter the **DevAddr** (4-byte device address from ABP configuration) converted to decimal.
   For example, if your DevAddr is *26011D01* (hex), convert it to decimal: 637534465.
   You can use an online hex-to-decimal converter or Python: ``int('26011D01', 16)``.
**Sensor Board:**
   Select **TTN**, to select use the small arrow on the right.


Enable TTN Storage Integration (optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to fetch historical data from TTN using the ``tools/ttn_fetcher`` scripts,
you need to enable the Storage Integration in TTN:

1. In your TTN Application, go to **Integrations** → **Storage Integration**
2. Click **Activate Storage Integration**
3. Data will be stored for up to 24 hours

This allows you to query uplink messages via the TTN API. See ``tools/ttn_fetcher/README.md``
for details on using the data fetcher daemon.

Webhook integration
~~~~~~~~~~~~~~~~~~~~

To get the data via TTN to *sensor.community*, the Webhook integration at TTN has to be activated.

At the Application tab select **Integrations** from the left menue. Select **Webhooks**, then **+ Add webhook**. 
Scroll down and select **Custom webhook**.

Fill in the following fields:

**Webhook ID**
  Enter an arbitrary name for this hook
**Webhook format**
  Select **JSON**
**Base URL**
  Enter **https://ttn2luft.citysensor.de**
**Downlink API key**
  This remains empty

Then click at **+ Add header entry** to add a special Header. Enter **X-SID** in the left field and your 
sensor id (the number, you received from sensor.community, **not** the chip ID) in the right field.
Enable **Uplink message**, all other selections remain disabled. Now click **Add webhook** (or **save changes**).

TTN Payload Decoder
~~~~~~~~~~~~~~~~~~~

In order to get readable values in the TTN console instead of solely data bytes, a payload decoder
can be configured. This decoder translates the binary payload into human-readable JSON data including
calculated values like CPM (counts per minute), CPS (counts per second), and dose rate in µSv/h.

Go to the TTN website, log in, click **Applications** to find the application you created above.
From the left menu select **Payload formatters** and then **Uplink**.

At **Formatter type** select **Javascript** and in **Formatter parameter** paste the following code
(replace existing code):

::

  function decodeUplink(input) {
    const bytes = input.bytes;
    const fPort = parseInt(input.fPort) || 0;

    if (fPort !== 1 || bytes.length !== 10) {
      return {
        data: {},
        warnings: [`fPort: ${fPort}, bytes: ${bytes.length}`],
        errors: []
      };
    }

    const COUNTS = bytes[0] * 0x1000000 + bytes[1] * 0x10000 + bytes[2] * 0x100 + bytes[3];
    const DT_MS = bytes[4] * 0x10000 + bytes[5] * 0x100 + bytes[6];
    const SW_VERSION = bytes[7] * 0x100 + bytes[8];
    const TUBE_NBR = bytes[9];

    const CPS = DT_MS > 0 ? COUNTS / (DT_MS / 1000) : 0;
    const CPM = DT_MS > 0 ? Math.round(COUNTS * 60000 / DT_MS * 10) / 10 : 0;
    const USVH = CPS / 12.2792;  // Conversion factor for Si22G tube

    return {
      data: {
        counts: COUNTS,
        cpm: CPM,
        cps: Math.round(CPS * 100) / 100,
        usvh: Number(USVH.toFixed(3)),
        sample_time_ms: DT_MS,
        tube_number: TUBE_NBR,
        sw_version: `V${(SW_VERSION >> 12) & 0x0F}.${(SW_VERSION >> 4) & 0xFF}.${SW_VERSION & 0x0F}`
      },
      warnings: [],
      errors: []
    };
  }

**Decoded data fields:**

- **counts**: Total number of GM tube pulses detected during the measurement interval
- **cpm**: Counts per minute (extrapolated from counts and sample time)
- **cps**: Counts per second (calculated)
- **usvh**: Dose rate in µSv/h (microsieverts per hour) using Si22G conversion factor
- **sample_time_ms**: Measurement interval in milliseconds
- **tube_number**: GM tube type identifier (0x13=SBM-19, 0x14=SBM-20, 0x16=Si22G)
- **sw_version**: MultiGeiger firmware version

.. note::
   The conversion factor 12.2792 CPS/µSv/h is specific to the Si22G tube. For other tube types,
   this factor needs to be adjusted according to the tube's calibration data.



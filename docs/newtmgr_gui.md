# Newtmgr GUI Requirements

The following data sets need to be converted into a GUI or displayed in an
appealing manner

# image

Image management GUI and behaviour TBD.

# stats

This lists groups of statistics for the system, and each group can be
expanded to see the individual statistics in the group.

### Raw data

The following raw data is generated by newtmgr. Text based names may or may not
be available. If names are disabled (to save space) number will be used like
`s0`, `s1`, `s2`, etc..

`stat stat` can be used to get the number of statistic groups available, and
common statistics groups are listed below (this assumes names are available):

- ble_ll_conn: Link layer connection statistics
- ble_ll: BLE link layer statistics
- stat: The number of statistic groups present
- ble_l2cap: L2CAP statistics
- ble_att: Bluetooth attribute table statistics
- ble_gap: GAP statistics
- ble_gattc: GATT characteritic statistics
- ble_gatts: GATT service statistics
- ble_hs
- ble_phy: BLE PHY statistics

Results of `$ newtmgr -c serial stat ble_gap`:

```
Stats Name: ble_gap
  adv_start: 1
  terminate_fail: 0
  rx_update_complete: 0
  rx_conn_complete: 0
  discover_cancel: 0
  wl_set: 0
  adv_start_fail: 0
  discover: 0
  discover_fail: 0
  update: 0
  adv_stop_fail: 0
  adv_set_fields: 1
  cancel: 0
  security_initiate: 0
  security_initiate_fail: 0
  cancel_fail: 0
  connect_mst: 0
  connect_slv: 0
  disconnect: 0
  wl_set_fail: 0
  rx_disconnect: 0
  initiate: 0
  initiate_fail: 0
  rx_adv_report: 0
  discover_cancel_fail: 0
  adv_rsp_set_fields: 1
  adv_stop: 0
  adv_set_fields_fail: 0
  adv_rsp_set_fields_fail: 0
  terminate: 0
  update_fail: 0
```

Results of `$ newtmgr -c serial stat ble_phy`:

```
Stats Name: ble_phy
  phy_isrs: 28582
  tx_good: 28556
  tx_fail: 0
  tx_late: 0
  tx_bytes: 913784
  rx_starts: 25
  rx_aborts: 23
  rx_valid: 2
  rx_crc_err: 0
  rx_late: 0
  radio_state_errs: 0
  rx_hw_err: 0
  tx_hw_err: 0
```

### Requirements

The main requirement here is simply a pleasing display of the data, and an
easy means to navigate through the individual groups.

- Entries come back in a random order that isn't consistent, and they should
  be sorted by name by default
- You should be able to refresh the statistics or enable updates every 'n'
  seconds.
- Have the ability to export stats as a file

### Design Issues

How should the groups be displayed and selected? As a drill down treeview type
list? In two columns where you select the group on the left and the individual
stats are rendered on the right?

### Example

If a two column approach is used we would have:

```
ble_ll_conn  | phy_isrs           28,582
ble_ll       | tx_good            28,556
stat         | tx_fail                 0
ble_l2cap    | tx_late                 0
ble_att      | tx_bytes          913,784
ble_gap      | ...
ble_gattc    |
ble_gatts    |
ble_hs       |
-------------+
ble_phy
-------------+
...          | ...
```

If a treeview approach is used we would have:

```
> ble_ll_conn
> ble_ll
> stat
> ble_l2cap
> ble_att
> ble_gap
> ble_gattc
> ble_gatts
> ble_hs
v ble_phy
  o phy_isrs          28,582
  o tx_good           28,556
  o tx_fail                0
  o tx_late                0
  o tx_bytes         913,784
  ...
...
```

Some other structure may be possible or desirable as well?

# taskstats

This lists a set of all currently executing tasks by name, as well as some
information about them:

### Raw data

The following raw data is generated by newtmgr:

```
ble_ll (prio=0 tid=6 runtime=3104 cswcnt=76205 stksize=80 stkusage=64 last_checkin=0 next_checkin=0)
idle (prio=255 tid=0 runtime=1630243 cswcnt=1701682 stksize=64 stkusage=26 last_checkin=0 next_checkin=0)
shell (prio=3 tid=1 runtime=0 cswcnt=2 stksize=384 stkusage=50 last_checkin=0 next_checkin=0)
newtmgr (prio=4 tid=2 runtime=0 cswcnt=2 stksize=512 stkusage=114 last_checkin=0 next_checkin=0)
blinky (prio=10 tid=3 runtime=0 cswcnt=1634 stksize=128 stkusage=26 last_checkin=0 next_checkin=0)
bleuart_bridge (prio=5 tid=4 runtime=0 cswcnt=1636383 stksize=256 stkusage=28 last_checkin=0 next_checkin=0)
bleprph (prio=1 tid=5 runtime=1 cswcnt=3302 stksize=336 stkusage=226 last_checkin=0 next_checkin=0)
```

Where the individual values are:

- prio = Task priority (lower value = higher priority)
- tid = Task ID
- runtime = Total task runtime
- cswcnt = Context switch count
- stksize = Total stack (memory) size
- stkusage = Total stack (memory) usage
- last_checkin = Last sanity checkin
- newt_checkin = Next sanity checkin

### Requirements

- Tasks should be sortable by any value, with the default being by name.
- Set the refresh rate in seconds and update the table accordingly, shifting
  values up or down if sorting is enabled.
- You should be able to refresh the statistics or enable updates every 'n'
  seconds.
- It might be nice to have the option to plot statistics as they change over
  time, although this will require a bit more work and logging historical data
  locally.

### Design Issues

How should the individual tasks be displayed and how should sorting be enabled?

Having a table with all the data allows easy sorting, but takes up a lot of
horizontal space and can cause visual clutter. Perhaps you should have the
option to click on a task to see extended information?

- Key info = Task Name, Priority, Runtime, Context Switches, Stack size/usage

Total stack size can be displayed as a value in bytes, and stack usage can be
a graphical progress bar element with the number of bytes used overlayed on top
of the progress bar, with the color changing as it approaches stack size.

Expanding a task then exposes additional information on the task:

- Task ID, Last Sanity Checkin, Next Sanity Checkin?

### Example

| Task             | Pri | Runtime | Switches | Stack Size | Stack Usage |
|------------------|----:|--------:|---------:|-----------:|------------:|
| > ble_ll         |   0 |    3104 |    76205 |         80 |          64 |
| > bleuart_bridge |   5 |       0 |  1636383 |        256 |          28 |
| > idle           | 255 | 1630243 |  1701682 |         64 |          26 |

Clicking on a task will then expand the entry to include all the rest of the
information, but this means we can only sort by the primary values.

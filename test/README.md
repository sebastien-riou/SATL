# SATL test

The core of the test is implemented by the file `satl_test.py`.
It is capable to handle master and slave roles.

## Python implementation tests

The python implementation can be tested for master and slave roles at the same time using the following scripts:
- `test_0_1_1.py`
- `test_0_2_2.py`
- `test_1_1_1.py`
- `test_268_4_4.py`

All of them are based on test_symmetric_case.py which is itslef relying on satl_test.py.

On linux the scripts tend to fail if launched one shortly after another because the port is still considered to be in use. This problem can be avoided by explicitely setting the port value:

````
./test_0_1_1.py  50000
./test_0_2_2.py  50001
...
````

## C99 implementation tests

Use python script under `basic/drivers/<driver_name>/test_<driver_name>.py`

## C99 vs Python implementation tests

To test the C99 as master, use `basic/drivers/<driver_name>/test_<driver_name>_master.py`
To test the C99 as slave, use `basic/drivers/<driver_name>/test_<driver_name>_slave.py`
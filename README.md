To get started, you should have vagrant set up and installed with virtual box. Because clang on linux is usually not new, and libc++ is nowhere to be found, this project contains a precompiled copy and will set up a development environment that supports the latest C++11 features this project uses.

Do `vagrant up` to set up the virtual machine. Following that, you may then ssh in and do the following to get started.


```
vagrant ssh
Welcome to Ubuntu 12.04 LTS (GNU/Linux 3.2.0-23-generic x86_64)

 * Documentation:  https://help.ubuntu.com/
Welcome to your Vagrant-built virtual machine.
Last login: Fri Sep 14 06:23:18 2012 from 10.0.2.2
vagrant@worker:~$ cd /vagrant/
vagrant@worker:/vagrant$ mkdir bin
vagrant@worker:/vagrant$ cd bin
vagrant@worker:/vagrant/bin$ CC=clang CXX=clang++ cmake ..
-- The CXX compiler identification is Clang
-- Check for working CXX compiler: /usr/local/bin/clang++
-- Check for working CXX compiler: /usr/local/bin/clang++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- The C compiler identification is Clang
-- Check for working C compiler: /usr/local/bin/clang
-- Check for working C compiler: /usr/local/bin/clang -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Configuring done
-- Generating done
-- Build files have been written to: /vagrant/bin
vagrant@worker:/vagrant/bin$ make
```
Once ready, you also need node set up so you can run a basic API server that talks over ZeroMQ to the `cserviced` application.

So start it in vagrant so it looks like

    vagrant@worker:/vagrant/bin$ ./service/cserviced

Then on the host, go to the serviceproxy folder and run

+ `npm install`
+ `node bootstrap.js`

Following that, you should be able to go to [http://localhost:8193/]() and then see `{"hello":"world"}`
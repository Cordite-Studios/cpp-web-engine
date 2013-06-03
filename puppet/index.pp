class repository {
  # We need cURL installed to import the key
  package { 'curl': ensure => installed }
  # Refreshes the list of packages
  exec { 'apt-get-update':
    command => 'apt-get update',
    path    => ['/bin', '/usr/bin'],
    onlyif => "/bin/bash -c 'exit $(( $(( $(date +%s) - $(stat -c %Y /var/lib/apt/lists/$( ls /var/lib/apt/lists/ -tr1|tail -1 )) )) <= 604800 ))'",
  }
}
# ...
 
stage { pre: before => Stage[main] }
 
class { 'repository':
  # Forces the repository to be configured before executing any other task
  stage => pre
}

class buildable {
	package { "build-essential":
		ensure => installed,
	}
	package { "cmake":
		ensure => installed,
	}
	package { "libpq-dev":
		ensure => installed,
	}
	package { "libhiredis-dev":
		ensure => installed,
	}
	package { "libjemalloc-dev":
		ensure => installed,
	}
	package { "language-pack-en":
		ensure => installed,
	}
	package { "valgrind":
		ensure => installed,
	}
	
	#Can't use the zeromq in the repos.
	#They haven't bothered to upgrade..
	file { "/root/zmq.tar.gz":
		ensure => present,
		source => "/vagrant/puppet/files/zeromq-3.2.2.tar.gz",
	}
	exec { "untar zmq":
		require => File["/root/zmq.tar.gz"],
		command => "/bin/tar xzvf /root/zmq.tar.gz",
		cwd => "/root/",
		unless => "/usr/bin/test -d /root/zeromq-3.2.2"
	}
	exec { "configure zmq":
		require => Exec["untar zmq"],
		command => "/root/zeromq-3.2.2/configure",
		cwd => "/root/zeromq-3.2.2/",
		unless => "/usr/bin/test -f /root/zeromq-3.2.2/config.status"
	}
	exec { "make zmq":
		require => Exec["configure zmq"],
		command => "/usr/bin/make",
		cwd => "/root/zeromq-3.2.2/",
		unless => "/usr/bin/test -f /root/zeromq-3.2.2/src/libzmq.la",
	}
	exec { "install zmq":
		require => Exec["make zmq"],
		command => "/usr/bin/make install",
		cwd => "/root/zeromq-3.2.2/",
		unless => "/usr/bin/test -f /usr/local/lib/libzmq.so",
	}
	file { "/etc/ld.so.conf.d/local.conf":
		ensure => present,
		source => "/vagrant/puppet/files/local.conf",
		require => Exec["install zmq"],
	}
	exec {"ldconfig":
		command => "/sbin/ldconfig",
		require => File["/etc/ld.so.conf.d/local.conf"],
	}
}
class developer {
	package { "subversion":
		ensure => installed,
	}
	file { "/root/clang.tar.gz":
		ensure => present,
		source => "/vagrant/puppet/files/clang.tar.gz"
	}
	exec { "extract clang":
		require => File["/root/clang.tar.gz"],
		command => "/bin/tar xzvf /root/clang.tar.gz",
		cwd => "/root/",
		unless => "/usr/bin/test -d /root/clang+llvm-3.2-x86_64-linux-ubuntu-12.04"
	}
	exec { "copy clang":
		require => Exec["extract clang"],
		command => "/bin/cp -r * /usr/local/",
		cwd => "/root/clang+llvm-3.2-x86_64-linux-ubuntu-12.04/",
		unless => "/usr/bin/test -f /usr/local/bin/clang"
	}
	exec { "get libc++":
		require => Package["subversion"],
		command => "/usr/bin/svn co http://llvm.org/svn/llvm-project/libcxx/trunk /root/libcxx",
		unless => "/usr/bin/test -d /root/libcxx/"
	}
	file { "/usr/include/c++/v1":
		require => Exec["get libc++"],
		source => "/root/libcxx/include",
		ensure => directory,
		recurse => true,
	}
	file { "/usr/lib/libc++.so.1.0":
		source => "/vagrant/puppet/files/libcxx/libc++.so.1.0"
	}
	file { "/usr/lib/libc++.so":
		ensure => link,
		source => "/usr/lib/libc++.so.1.0",
		require => File["/usr/lib/libc++.so.1.0"],
	}
	file { "/usr/lib/libc++.so.1":
		ensure => link,
		source => "/usr/lib/libc++.so.1.0",
		require => File["/usr/lib/libc++.so.1.0"],
	}
	file { "/usr/lib/libcxxrt.so":
		source => "/vagrant/puppet/files/libcxx/libcxxrt.so"
	}
	file { "/usr/lib/unwind.h":
		source => "/vagrant/puppet/files/libcxx/unwind.h"
	}
	file { "/usr/lib/unwind-itanium.h":
		source => "/vagrant/puppet/files/libcxx/unwind-itanium.h"
	}
}
node "worker" {
	include developer
	include buildable
}
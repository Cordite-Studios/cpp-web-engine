Vagrant::Config.run do |config|
	config.vm.box = "precise64"
	config.vm.box_url = "http://files.vagrantup.com/precise64.box"

	config.vm.define :worker do |i_config|
		i_config.vm.forward_port 80, 10080
		i_config.vm.customize ["modifyvm", :id,
			 "--memory", 512,
			 "--cpus", 1
			]
		i_config.vm.host_name = "worker"
		i_config.vm.network :hostonly, "192.168.5.200"
		i_config.vm.provision :puppet do |puppet|
			puppet.manifests_path = "puppet"
			puppet.manifest_file = "index.pp"
		end
	end
end

Given /^superNET is running on the computer$/ do
    # Start nxt client if not already running or throw an exception
    raise 'NXT server is not running' unless `ps aux | grep -i nxt.nxt | grep -v grep` != ""
    # Start BicoinDarkd if not already running or throw an exception
    App.start unless `ps aux | grep -i BitcoinDarkd | grep -v grep` != ""
    raise 'BitcoinDarkd is not running' unless `ps aux | grep -i BitcoinDarkd | grep -v grep` != ""
end

When /^I receive a list of peers with at least two peers$/ do
    request = '{ "requestType": "getpeers" }'
    peers_resp = SuperNETTest.requestSuperNET($rpcuser, $rpcpassword, request)
    raise 'No Numpservers variable found in getpeers response' unless peers_resp.include?('Numpservers')
    raise 'No peers found on the network' unless peers_resp['Numpservers'].to_i > 1
    raise 'No peers found on the network' unless peers_resp['peers'].size > 1
    $peers = peers_resp['peers']
end

Given /^superNET is running on the computer with some peers$/ do
    step 'superNET is running on the computer'
    step 'I receive a list of peers with at least two peers'
end

When /^I send a ping request to one random peer$/ do
    raise 'Not enough peers found on the network' unless $peers.size > 2
    peer_ip = $peers[rand(2..($peers.size-1))]['srvipaddr']
    $ping_requested_ip_address = peer_ip
    request = "{\"requestType\":\"ping\",\"destip\":\"#{peer_ip}\"}"
    SuperNETTest.requestSuperNET($rpcuser, $rpcpassword, request)
end

When /^I send a ping request to "(.*)" peer$/ do |peer_ip|
    raise 'Not enough peers found on the network' unless $peers.size > 2
    $ping_requested_ip_address = peer_ip
    request = "{\"requestType\":\"ping\",\"destip\":\"#{peer_ip}\"}"
    SuperNETTest.requestSuperNET($rpcuser, $rpcpassword, request)
end

Then /^I receive a pong answer( in less than (\d+) seconds)?$/ do |with_timeout, seconds|
    timeout = with_timeout ? Integer(seconds) : 50
    raise 'Missing ping IP before pong response' if $ping_requested_ip_address.nil?
    App.searchLog("PONG.*#{$ping_requested_ip_address}", false, timeout)
end

When /^I send a message to myself with content "(.*)"$/ do |text|
    my_nxt_address = "7837143510182070614"
    request = "{\"requestType\":\"sendmessage\",\"dest\":\"#{my_nxt_address}\",\"msg\":\"#{text}\"}"
    SuperNETTest.requestSuperNET($rpcuser, $rpcpassword, request)
end

Then /^I receive a message to myself with content "(.*)"( in less than (\d+) seconds)?$/ do |text, with_timeout, seconds|
    timeout = with_timeout ? Integer(seconds) : 10
    App.searchLog('GOT MESSAGE.('+text, true, timeout)
end
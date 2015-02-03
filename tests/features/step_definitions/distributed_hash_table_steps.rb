When /^I request one of my peers for information to find more nodes$/ do
    peer_nxt_key = $peers[rand(2..($peers.size-1))]['srvNXT']
    request = "{\"requestType\":\"findnode\",\"key\":\"#{peer_nxt_key}\"}"
    SuperNETTest.requestSuperNET($rpcuser, $rpcpassword, request)
end

Then /^I see a request for information to find more nodes in the log$/ do
    App.searchLog('FIND.({"result":"kademlia_findnode', true)
end
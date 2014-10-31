var authToken = btoa("user:simplepass");
var rpcport = "14632";

function initRPC(data) {
    $.ajax({
        url: 'http://127.0.0.1:' + rpcport,
        dataType: 'json',
        type: 'POST',
        crossDomain: true,
        xhrFields: {
            withCredentials: true
        },
        headers: { 'Authorization': 'Basic ' + authToken },
        contentType: 'application/json',
        data: data,
        timeout: 10000,
        success: function (data) {
            showResult(JSON.stringify(data.result));
        },
        error: function (jqXHR) {
            showResult("RPC status : " + jqXHR.status + " ( " + jqXHR.statusText + " )");
        }
    });
}

function initNxtApi(qs) {
    $.ajax({
        url: 'http://127.0.0.1:7876/nxt?' + qs,
        dataType: 'json',
        type: 'GET',
        crossDomain: true,
        contentType: 'application/json',
        timeout: 10000,
        success: function (data) {
            showResult(JSON.stringify(data));
        },
        error: function (jqXHR) {
            showResult("NXT API status : " + jqXHR.status + " ( " + jqXHR.statusText + " )");
        }
    });
}

function initSuperNETrpc(params) {
    $.ajax({
        url: 'http://127.0.0.1:' + rpcport,
        dataType: 'json',
        type: 'POST',
        xhrFields: {
            withCredentials: true
        },
        headers: { 'Authorization': 'Basic ' + authToken },
        crossDomain: true,
        contentType: 'application/json',
        data: JSON.stringify({
            method: "SuperNET",
            params: [params]
        }),
        timeout: 10000,
        success: function (data) {
            showResult(data.result);
        },
        error: function (jqXHR) {
            showResult("RPC status : " + jqXHR.status + " ( " + jqXHR.statusText + " )");
        }
    });
}

$("#btnGetBlock").click(function () {
    initRPC('{"method":"getblockcount"}');
});

$("#btnGetAddrPrivKey").click(function () {
    var address = $("#btcdAddr").val();
    initRPC('{"method": "dumpprivkey", "params":["' + address + '"]}');
}); 

$("#btnGetState").click(function () {
    initNxtApi("requestType=getState");
});
$("#btnPing").click(function () {
    initSuperNETrpc("{\"requestType\":\"ping\",\"destip\":\"" + $("#pingAddr").val() + "\"}");
});
$("#btnDisplayContact").click(function () {
    initSuperNETrpc("{\"requestType\":\"dispcontact\",\"contact\":\"*\"}");
});
$("#btnAddContact").click(function () {
    initSuperNETrpc("{\"requestType\":\"addcontact\",\"handle\":\"" + $("#contactHandle").val() + "\",\"acct\":\"" + $("#contactAddr").val() + "\"}");
});
$("#btnRemoveContact").click(function () {
    initSuperNETrpc("{\"requestType\":\"removecontact\",\"contact\":\"" + $("#removeContactAddr").val() + "\"}");
});
$("#btnGetPeers").click(function () {
    initSuperNETrpc("{\"requestType\":\"getpeers\"}");
});
$("#btnTelepathy").click(function () {
    initSuperNETrpc("{\"requestType\":\"telepathy\",\"contact\":\"" + $("#telepathyContact").val() + "\",\"msg\":\"" + $("#telepathyMsg").val() + "\"}");
});

function showResult(result) {
    $("#result").html(result);
}

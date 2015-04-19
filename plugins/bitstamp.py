from ws4py.client.tornadoclient import TornadoWebSocketClient
from tornado import ioloop
import json
import urllib
ws = None


class MyClient(TornadoWebSocketClient):

    def sendEvent(self, eventType, eventData):
        msgDict = {"event": eventType}
        if eventData:
            msgDict["data"] = eventData
        msg = json.dumps(msgDict)
        self.send(msg, False)

    def opened(self):
        self.sendEvent("pusher:subscribe", {"channel": "live_trades"})
        #self.sendEvent("pusher:subscribe", {"channel": "order_book"})
        #self.sendEvent("pusher:subscribe", {"channel": "diff_order_book"})

    def received_message(self, m):
        print(m)

    def closed(self, code, reason=None):
        print(reason)
        print(code)
        ioloop.IOLoop.instance().stop()
    

def ws_thread():

    global ws
    params = {"protocol": 5, "client": "SuperNET","version": "1.9.3"}
    #wsurl = 'wss://real.okcoin.com:10440/websocket/okcoinapi'
    wsurl = "wss://ws.pusherapp.com/app/de504dc5763aeef9ff52?%s" %(urllib.parse.urlencode(params))
    ws = MyClient(wsurl, protocols=['wamp.2.json'])
    ws.connect()
    ioloop.IOLoop.instance().start()


if __name__ == '__main__':

    ws_thread()
    #try:
    #    thread.start_new_thread(ws_thread, ("wsthread",1))
    #except:
    #    print('fail')

    #while True:
    #    time.sleep(1)

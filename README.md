# SimQ
Simple message queue system

The system is written under Linux. Features:
- simple administration both via the console and via the API
- polling of messages from the consumer
- maximum message length is 4Gb
- the ability to store messages both in memory and on disk
- ability to replicate messages
- setting queue limits (message length, queue size)
- implemented a simple proprietary binary protocol
- the ability to send a signal to consumers who are connected to the queue
- minimal dependence on third-party libraries (OpenSSL)

Read more [here](https://simq.org/guide) 

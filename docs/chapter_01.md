# Chapter 1: The Handshake and the Ping-Pong

## Introduction

<div class="callout callout-info"><strong>Note:</strong><br> At the end of the chapter there are code snippets and GitHub links.</div>

The Noso protocol is quite a simple one:

1. The node connects to another node using an always on `TCP` stream connection.
2. Sends an initial [Hello line](#the-hello).
3. It then engages in sending a [Ping line](#the-ping) every 5 seconds.

Each message sent is also quite simple: They are composed of a string with space separated values terminated by and End-Of-Line(`EOL`) sequence.

## The Flow

According to the source code one should send the [hello](#the-hello), immediately followed by the [ping](#the-ping).

The [ping](#the-ping) should then be sent on a continuous loop of 5 seconds interval.

<div class="callout callout-warning"><strong>Note:</strong><br> Need to find out more about when to expect the pong.</div>

This is the data that comes out of a node trying to connect:

```console
$ nc -l 8080
PSK 10.42.0.68 0.3.2Ab8 1659944140
PSK 2 0.3.2Ab8 1659944140 $PING 4 69773 6D5FC7634CA132803A480928DBA4AED8 4DB6D1C46E0DD804923167BB807840F3 4 650D27A438E4E3E6264A079BC3EE6A12 3 -1 F35B4 163 00000000052841423044B5C791445E22 0 0B5866B63B12A3A083C72BD4BA4F330E 5EDA6
```

The first line is the hello.  
That last line is repeated 3 more times(5 seconds interval), for a total of 15-16 seconds, until the wallet closes the connection due to perceived inactivity.

## The Hello

This is a space separated string with the following data/fields:

0. The string `PSK`
1. The target IP address: String
2. The version of the software, `0.3.2Ab8` at this moment: String
3. The current UTC Unix timestamp: Integer

Format string: `PSK %s %s %d`

## The Ping

This is a space separated string with the following data/fields:

0. The string `PSK`
1. The current protocol version `2`: Integer
2. The version of the software, `0.3.2Ab8` at this moment: String
3. The current UTC Unix timestamp: Integer
4. The string `$PING`: String
5. The current amount of connections: Integer
6. The current block number(Default: 0): Integer
7. The current block hash(Default: `4E8A4743AA6083F3833DDA1216FE3717`): String
8. The hash of the `NOSODATA/sumary.psk` file(Default: `D41D8CD98F00B204E9800998ECF8427E`): String
9. The amount of pending orders(Default: 0): Integer
10. The hash of the `NOSODATA/blchhead.nos` file(Default: `D41D8CD98F00B204E9800998ECF8427E` ??): String
11. Status of the connection: Integer
    - 0: Disconnected
    - 1: Connecting
    - 2: Connected
    - 3: Updated
12. Connection port(Default: 8080): Integer
13. First five characters of the hash of the `NOSODATA/masternodes.txt` file(Default: `D41D8`): String
14. The amount of Master Nodes(Default: 0): Integer
15. NMsData difference(Need to ask what this means)(Default: ??): String
16. The amount of checked Master Nodes(Default: 0): Integer
17. Hash of the `NOSODATA/gvts.psk` file(Default: `D41D8CD98F00B204E9800998ECF8427E`): String
18. First five characters of the hash of the CFGs(Need to ask what this means)(Default: `D41D8` ??): String

Format string: `PSK %d %s %d $PING %d %d %s %s %d %s %d %d %s %d %s %d %s %s`

<div class="callout">
    <code>4E8A4743AA6083F3833DDA1216FE3717</code> is the value of the hash for Block 0(zero).<br>
    <code>D41D8CD98F00B204E9800998ECF8427E</code> is the value of <code>md5</code> on an empty string.
</div>

## The Pong

This is a space separated string with the following data/fields:

0. The string `PSK`
1. The current protocol version `2`: Integer
2. The version of the software, `0.3.2Ab8` at this moment: String
3. The current UTC Unix timestamp: Integer
4. The string `$PONG`: String
5. The current amount of connections: Integer
6. The current block number(Default: 0): Integer
7. The current block hash(Default: `4E8A4743AA6083F3833DDA1216FE3717`): String
8. The hash of the `NOSODATA/sumary.psk` file(Default: `D41D8CD98F00B204E9800998ECF8427E`): String
9. The amount of pending orders(Default: 0): Integer
10. The hash of the `NOSODATA/blchhead.nos` file(Default: `D41D8CD98F00B204E9800998ECF8427E` ??): String
11. Status of the connection: Integer
    - 0: Disconnected
    - 1: Connecting
    - 2: Connected
    - 3: Updated
12. Connection port(Default: 8080): Integer
13. First five characters of the hash of the `NOSODATA/masternodes.txt` file(Default: `D41D8`): String
14. The amount of Master Nodes(Default: 0): Integer
15. NMsData difference(Need to ask what this means)(Default: ??): String
16. The amount of checked Master Nodes(Default: 0): Integer
17. Hash of the `NOSODATA/gvts.psk` file(Default: `D41D8CD98F00B204E9800998ECF8427E`): String
18. First five characters of the hash of the CFGs(Need to ask what this means)(Default: `D41D8` ??): String

Format string: `PSK %d %s %d $PONG %d %d %s %s %d %s %d %d %s %d %s %d %s %s`

<div class="callout">
    <code>4E8A4743AA6083F3833DDA1216FE3717</code> is the value of the hash for Block 0(zero).<br>
    <code>D41D8CD98F00B204E9800998ECF8427E</code> is the value of <code>md5</code> on an empty string.
</div>

## Code and Links

> File: mpred.pas  
> Function: ConnectClient  
> Line: 491  
> Commit: e0787b818f3b637971f027480d29a3538b2599b8  
> Link: [mpred.pas@e0787b8#491](https://github.com/Noso-Project/NosoWallet/blob/e0787b818f3b637971f027480d29a3538b2599b8/mpred.pas#L491)

```pas
function ConnectClient(Address,Port:String):integer;
{...}
  CanalCliente[Slot].IOHandler.WriteLn('PSK '+Address+' '+ProgramVersion+subversion+' '+UTCTime);
  CanalCliente[Slot].IOHandler.WriteLn(ProtocolLine(3));   // Send PING
{...}
```

> File: mpprotocol.pas  
> Function: ProtocolLine  
> Line: 186  
> Commit: e0787b818f3b637971f027480d29a3538b2599b8  
> Link: [mpprotocol.pas@e0787b8#186](https://github.com/Noso-Project/NosoWallet/blob/e0787b818f3b637971f027480d29a3538b2599b8/mpprotocol.pas#L186)

```pas
Function ProtocolLine(tipo:integer):String;
{...}
if tipo = Ping then
   Resultado := '$PING '+GetPingString;
if tipo = Pong then
   Resultado := '$PONG '+GetPingString;
{...}
```

> File: mpprotocol.pas  
> Function: GetPingString  
> Line: 513  
> Commit: e0787b818f3b637971f027480d29a3538b2599b8  
> Link: [mpprotocol.pas@e0787b8#513](https://github.com/Noso-Project/NosoWallet/blob/e0787b818f3b637971f027480d29a3538b2599b8/mpprotocol.pas#L513)

```pas
function GetPingString():string;
{...}
result :=IntToStr(GetTotalConexiones())+' '+
         IntToStr(MyLastBlock)+' '+
         MyLastBlockHash+' '+
         MySumarioHash+' '+
         GetPendingCount.ToString+' '+
         MyResumenHash+' '+
         IntToStr(MyConStatus)+' '+
         IntToStr(port)+' '+
         copy(MyMNsHash,0,5)+' '+
         IntToStr(GetMNsListLength)+' '+
         GetNMSData.Diff+' '+
         GetMNsChecksCount.ToString+' '+
         MyGVTsHash+' '+
         Copy(HashMD5String(GetNosoCFG),0,5);
{...}
```

# Annex 1: The Block

## The Block Header

> File: masterpaskalform.pas  
> Line: 171  
> Commit: e0787b818f3b637971f027480d29a3538b2599b8  
> Link: [masterpaskalform.pas@e0787b8#171](https://github.com/Noso-Project/NosoWallet/blob/e0787b818f3b637971f027480d29a3538b2599b8/masterpaskalform.pas#L171)
```pas
  BlockHeaderData = Packed Record
     Number         : Int64;
     TimeStart      : Int64;
     TimeEnd        : Int64;
     TimeTotal      : integer;
     TimeLast20     : integer;
     TrxTotales     : integer;
     Difficult      : integer;
     TargetHash     : String[32];
     Solution       : String[200]; // 180 necessary
     LastBlockHash  : String[32];
     NxtBlkDiff     : integer;
     AccountMiner   : String[40];
     MinerFee       : Int64;
     Reward         : Int64;
     end;
```

### Number

The number of the block.

`Type`: Integer 16 bits  
`Size`: 8 bytes

### TimeStart

The time in Unix timestamp when the block was started

`Type`: Integer 16 bits  
`Size`: 8 bytes

### TimeEnd

The time in Unix timestamp when the block was minted/built/finalised

`Type`: Integer 16 bits  
`Size`: 8 bytes

## The Order/Transfer Header

> File: masterpaskalform.pas  
> Line: 188  
> Commit: e0787b818f3b637971f027480d29a3538b2599b8  
> Link: [masterpaskalform.pas@e0787b8#188](https://github.com/Noso-Project/NosoWallet/blob/e0787b818f3b637971f027480d29a3538b2599b8/masterpaskalform.pas#L188)
```pas
  OrderData = Packed Record
     Block : integer;
     OrderID : String[64];
     OrderLines : Integer;
     OrderType : String[6];
     TimeStamp : Int64;
     Reference : String[64];
       TrxLine : integer;
       Sender : String[120];    // La clave publica de quien envia
       Address : String[40];
       Receiver : String[40];
       AmmountFee : Int64;
       AmmountTrf : Int64;
       Signature : String[120];
       TrfrID : String[64];
     end;
```

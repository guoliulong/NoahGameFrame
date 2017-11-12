echo on
protogen -i:NFDefine.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFDefine.cs
protogen -i:NFMsgBase.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFMsgBase.cs
protogen -i:NFMsgPreGame.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFMsgPreGame.cs
protogen -i:NFMsgShare.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFMsgShare.cs
protogen -i:NFFleetingDefine.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFFleetingDefine.cs
protogen -i:NFMsgURl.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFMsgURl.cs
protogen -i:NFMsgMysql.proto -o:../../NFTools/NFMessageLogViewer/MessageDefine/NFMsgMysql.cs

pause
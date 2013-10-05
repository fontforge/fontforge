'
' See the below variables, in particular helplink
' and the calls to addline() which build the core text
' of the dialog. The dialog is not HTML and can't be
' formatted.
'
first = 0
Sub addline( extra )
  If Len(bodytext) = 0 Then
     bodytext = extra
  Else
     bodytext = bodytext & chr(13) & extra
  End If
End Sub


dialogtitle = "FontForge: startup..."
helplink = "http://www.fontforge.org/ms-binary-startup.html"
bodytext = ""
addline( "First, check that you have Xming running..." )
addline( " " )
addline( "Would you like to see the troubleshooting guide?" )


ans = MsgBox( bodytext, (vbQuestion Or vbYesNo), dialogtitle )
If ans = vbYes Then
    Set objShell = CreateObject("Wscript.Shell")
    objShell.Run(helplink)
End If

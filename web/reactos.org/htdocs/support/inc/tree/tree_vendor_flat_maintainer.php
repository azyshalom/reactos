<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}


	if (usrfunc_IsModerator($RSDB_intern_user_id)) {
	
		$query_maintainer_vendor = mysql_query("SELECT * 
									FROM `rsdb_item_vendor` 
									WHERE `vendor_visible` = '1'
									AND `vendor_id` = '".mysql_real_escape_string($RSDB_SET_vendor)."'
									LIMIT 1 ;") ;
		$result_maintainer_vendor = mysql_fetch_array($query_maintainer_vendor);

		$RSDB_referrer="";
		$RSDB_usragent="";
		$RSDB_ipaddr="";
		if (array_key_exists('HTTP_REFERER', $_SERVER)) $RSDB_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
		if (array_key_exists('HTTP_USER_AGENT', $_SERVER)) $RSDB_usragent=htmlspecialchars($_SERVER['HTTP_USER_AGENT']);
		if (array_key_exists('REMOTE_ADDR', $_SERVER)) $RSDB_ipaddr=htmlspecialchars($_SERVER['REMOTE_ADDR']);

		$RSDB_TEMP_pmod = "";
		$RSDB_TEMP_txtreq1 = "";
		$RSDB_TEMP_txtreq2 = "";
		$RSDB_TEMP_txtspam = "";
		$RSDB_TEMP_verified = "";
		$RSDB_TEMP_vendname = "";
		$RSDB_TEMP_fullname = "";
		$RSDB_TEMP_txturl = "";
		$RSDB_TEMP_txtemail = "";
		$RSDB_TEMP_txtinfo = "";
		if (array_key_exists("pmod", $_POST)) $RSDB_TEMP_pmod=htmlspecialchars($_POST["pmod"]);
		if (array_key_exists("txtreq1", $_POST)) $RSDB_TEMP_txtreq1=htmlspecialchars($_POST["txtreq1"]);
		if (array_key_exists("txtreq2", $_POST)) $RSDB_TEMP_txtreq2=htmlspecialchars($_POST["txtreq2"]);
		if (array_key_exists("txtspam", $_POST)) $RSDB_TEMP_txtspam=htmlspecialchars($_POST["txtspam"]);
		if (array_key_exists("verified", $_POST)) $RSDB_TEMP_verified=htmlspecialchars($_POST["verified"]);
		if (array_key_exists("vendname", $_POST)) $RSDB_TEMP_vendname=htmlspecialchars($_POST["vendname"]);
		if (array_key_exists("fullname", $_POST)) $RSDB_TEMP_fullname=htmlspecialchars($_POST["fullname"]);
		if (array_key_exists("txturl", $_POST)) $RSDB_TEMP_txturl=htmlspecialchars($_POST["txturl"]);
		if (array_key_exists("txtemail", $_POST)) $RSDB_TEMP_txtemail=htmlspecialchars($_POST["txtemail"]);
		if (array_key_exists("txtinfo", $_POST)) $RSDB_TEMP_txtinfo=htmlspecialchars($_POST["txtinfo"]);

		// Edit application group data:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_SET_sec == "vendor" && $RSDB_TEMP_vendname != "" && $RSDB_TEMP_fullname != "" && $RSDB_TEMP_txturl != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			// Submit vendor entry:
				
			$update_group_entry = "INSERT INTO `rsdb_item_vendor` ( `vendor_id` , `vendor_name` , `vendor_visible` , `vendor_fullname` , `vendor_url` , `vendor_email` , `vendor_infotext` , `vendor_usrid` , `vendor_usrip` , `vendor_date` , `vendor_checked` ) 
									VALUES ('', '". mysql_real_escape_string($RSDB_TEMP_vendname) ."', '1', '". mysql_real_escape_string($RSDB_TEMP_fullname) ."', '". mysql_real_escape_string($RSDB_TEMP_txturl) ."', '". mysql_real_escape_string($RSDB_TEMP_txtemail) ."', '". mysql_real_escape_string($RSDB_TEMP_txtinfo) ."', '".mysql_real_escape_string($RSDB_intern_user_id)."', '".mysql_real_escape_string($RSDB_ipaddr)."', NOW( ) , 'yes')";
			mysql_query($update_group_entry);
			
			add_log_entry("low", "tree_vendor", "submit", "[Vendor] Submit entry", @usrfunc_GetUsername($RSDB_intern_user_id)." submitted the following vendor: \n\nVendor-Name: ".htmlentities($RSDB_TEMP_vendname)."\n\Fullname: ".htmlentities($RSDB_TEMP_fullname)." \n\nUrl: ".htmlentities($RSDB_TEMP_txturl)." \n\E-Mail: ".htmlentities($RSDB_TEMP_txtemail)." \n\Info: ".htmlentities($RSDB_TEMP_txtinfo), "0");
			?>
			<script language="JavaScript">
				window.setTimeout('window.location.href="<?php echo $RSDB_intern_link_db_sec_javascript2; ?>"','500')
			</script>
			<?php
		}

		// Special request:
		if ($RSDB_TEMP_pmod == "ok" && $RSDB_TEMP_txtreq1 != "" && $RSDB_TEMP_txtreq2 != "" && usrfunc_IsModerator($RSDB_intern_user_id)) {
			$report_submit="INSERT INTO `rsdb_logs` ( `log_id` , `log_date` , `log_usrid` , `log_usrip` , `log_level` , `log_action` , `log_title` , `log_description` , `log_category` , `log_badusr` , `log_referrer` , `log_browseragent` , `log_read` , `log_taskdone_usr` ) 
							VALUES ('', NOW( ) , '".mysql_real_escape_string($RSDB_intern_user_id)."', '".mysql_escape_string($RSDB_ipaddr)."', 'low', 'request', '".mysql_escape_string($RSDB_TEMP_txtreq1)."', '".mysql_escape_string($RSDB_TEMP_txtreq2)."', 'user_moderator', '0', '".mysql_escape_string($RSDB_referrer)."', '".mysql_escape_string($RSDB_usragent)."', ';', '0');";
			$db_report_submit=mysql_query($report_submit);
		}

?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="maintainer">
	  <tbody>
		<tr>
		  <td><p><b><a name="maintainerbar"></a>Maintainer: </b>
			  <a href="javascript:Show_vendentry()">Submit new vendor</a> | <a href="javascript:Show_requests()">Special requests</a></p>
		    <div id="vendentry" style="display: block">
			<fieldset>
			<legend>Edit vendor data</legend>
				<div align="left">
				  <form name="form1" method="post" action="<?php echo $RSDB_intern_link_db_sec.$RSDB_SET_sec."#maintainerbar"; ?>">
				      <p><font size="2">Vendor </font><font size="2">name: 
		                  <input name="vendname" type="text" id="vendname" size="40" maxlength="100">
			          (max. 100 chars)	<br>
			          <br>
			          Vendor fullname:
                      <input name="fullname" type="text" id="fullname" size="70" maxlength="255">
			          (max. 255 chars) <br>
			          <br>
			          URL:										        
			          <input name="txturl" type="text" id="txturl" size="70" maxlength="255">
			          (max. 255 chars) <br>
			            <br>
			          E-Mail:												
			          <input name="txtemail" type="text" id="txtemail" size="70" maxlength="100">
						(max. 100 chars)		            </font></p>
				      <p><font size="2">Information:<br>
                      <textarea name="txtinfo" cols="70" rows="5" id="txtinfo"></textarea>
				      <input name="pmod" type="hidden" id="pmod" value="ok">
				        <br>
		              <br>
		                <br>
		                <input type="submit" name="Submit" value="Save">	
	                  </font>				  
				              </p>
				  </form>
				</div>
			</fieldset>
		</div>
		<div id="requests" style="display: block">
			<fieldset><legend>Special requests</legend>
				<div align="left">
				  <form name="form4" method="post" action="<?php echo $RSDB_intern_link_vendor.$RSDB_SET_vendor."#maintainerbar"; ?>">
				    <p><font size="2">Message title:<br> 
		            <input name="txtreq1" type="text" id="txtreq1" size="40" maxlength="100">
				    </font></p>
				    <p><font size="2">Text:<br> 
		              <textarea name="txtreq2" cols="70" rows="5" id="txtreq2"></textarea>
</font><font size="2" face="Arial, Helvetica, sans-serif">
<input name="pmod" type="hidden" id="pmod" value="ok">
</font><font size="2">                    </font></p>
				    <p><font size="2"><strong>Note:</strong><br>
			        Please do NOT misuse this function. All administrators will be able to see your message and one of them may contact you per forum private message, email or just do the task you suggested/requested.</font></p>
				    <p><font size="2">If you want to ask something, or the task needs (in all the circumstances) a feedback,  use the website forum, the #reactos-web IRC channel, the mailing list or the forum private message system instead. </font></p>
				    <p><font size="2">This form is not a bug tracking tool nor a feature request function! Use <a href="http://www.reactos.org/bugzilla/">bugzilla</a> for such things instead!</font></p>
				    <p><font size="2"><strong>A sample usage for this form:</strong><br>
			        If you need a new category which doesn't exist, then write a request and one of the admins will read it and may add the missing category. Then you will be able to move this application group to the right category (if you have placed the application somewhere else temporary).</font></p>
				    <p>
				      <font size="2">
				      <input type="submit" name="Submit4" value="Submit">
                      </font> </p>
				  </form>
				</div>
			</fieldset>
		</div>
		  </td>
		</tr>
	  </tbody>
	</table>
	<script language="JavaScript1.2">

		document.getElementById('vendentry').style.display = 'none';
		document.getElementById('requests').style.display = 'none';
	
		function Show_vendentry()
		{
			document.getElementById('vendentry').style.display = (document.getElementById('vendentry').style.display == 'none') ? 'block' : 'none';
			document.getElementById('requests').style.display = 'none';
		}
		
		function Show_requests()
		{
			document.getElementById('vendentry').style.display = 'none';
			document.getElementById('requests').style.display = (document.getElementById('requests').style.display == 'none') ? 'block' : 'none';
		}

	</script>
<?php
	}
?>

<br />

<?php
	if (usrfunc_IsAdmin($RSDB_intern_user_id)) {
	
		$RSDB_TEMP_padmin = "";
		$RSDB_TEMP_done = "";
		if (array_key_exists("padmin", $_POST)) $RSDB_TEMP_padmin=htmlspecialchars($_POST["padmin"]);
		if (array_key_exists("done", $_POST)) $RSDB_TEMP_done=htmlspecialchars($_POST["done"]);
		
		if ($RSDB_TEMP_padmin == "ok" && $RSDB_TEMP_done != "" && usrfunc_IsAdmin($RSDB_intern_user_id)) {
			$update_log_entry = "UPDATE `rsdb_logs` SET 
									`log_taskdone_usr` = '". mysql_real_escape_string($RSDB_intern_user_id) ."' WHERE `log_id` = '". mysql_real_escape_string($RSDB_TEMP_done) ."' LIMIT 1 ;";
			mysql_query($update_log_entry);
		}
		
?>
	<table width="100%" border="0" cellpadding="0" cellspacing="0" class="admin">
	  <tr>
		<td><b><a name="adminbar"></a>Admin: </b><a href="javascript:Show_readrequests()">Read special requests</a> | <font size="1">all other functions are under construction ...
        </font>
		<div id="readrequests" style="display: block">
			<fieldset><legend>Read special requests</legend>

 <table width="100%" border="1">  
    <tr><td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Date</font></strong></font></div></td> 
    <td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">User</font></strong></font></div></td> 
    <td width="25%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Title</font></strong></font></div></td> 
    <td width="45%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Request</font></strong></font></div></td> 
    <td width="10%"><div align="center"><font color="#000000"><strong><font size="2" face="Arial, Helvetica, sans-serif">Done?</font></strong></font></div></td>
    </tr> <?php
					$cellcolor1="#E2E2E2";
					$cellcolor2="#EEEEEE";
					$cellcolorcounter="0";
					$query_entry_sprequest = mysql_query("SELECT * 
							FROM `rsdb_logs` 
							WHERE `log_level` LIKE 'low'
							AND `log_action` LIKE 'request'
							AND `log_category` LIKE 'user_moderator'
							ORDER BY `log_date` DESC
							LIMIT 0, 30;") ;
					while($result_entry_sprequest = mysql_fetch_array($query_entry_sprequest)) {
				?> 
  <tr valign="top" bgcolor="<?php
					$cellcolorcounter++;
					if ($cellcolorcounter == "1") {
						echo $cellcolor1;
						$color = $cellcolor1;
					}
					elseif ($cellcolorcounter == "2") {
						$cellcolorcounter="0";
						echo $cellcolor2;
						$color = $cellcolor2;
					}
				 ?>"> 
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo $result_entry_sprequest['log_date'];  if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></div></td> 
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo @usrfunc_GetUsername($result_entry_sprequest['log_usrid']); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></div></td> 
    <td><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo htmlentities($result_entry_sprequest['log_title']); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></td> 
    <td><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "<strike>"; } echo wordwrap(nl2br(htmlentities($result_entry_sprequest['log_description'], ENT_QUOTES))); if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo "</strike>"; } ?></font></td> 
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php if ($result_entry_sprequest['log_taskdone_usr'] != 0) { echo @usrfunc_GetUsername($result_entry_sprequest['log_taskdone_usr']); } 
		
		else {
	?>
        <form name="form5" method="post" action="<?php echo $RSDB_intern_link_group_group2_both."#adminbar"; ?>">
          <input type="submit" name="Submit5" value="Done!">
          <input name="padmin" type="hidden" id="padmin" value="ok">
          <font size="2" face="Arial, Helvetica, sans-serif">
          <input name="done" type="hidden" id="done" value="<?php echo $result_entry_sprequest['log_id']; ?>">
          </font>
        </form>
    <?php
		}
	
	 ?>
        </font></div></td>
  </tr> 
	<?php
		}
	?> 
</table>

			</fieldset>
		</div>		</td>
	  </tr>
	</table>
	<script language="JavaScript1.2">

		document.getElementById('readrequests').style.display = 'none';
	
		function Show_readrequests()
		{
			document.getElementById('readrequests').style.display = (document.getElementById('readrequests').style.display == 'none') ? 'block' : 'none';
		}
	</script>
<?php
	}
?>
<?php
// If not in Include mode: Perform test!
if(!$C4HostTestIncludeMode)
	if(performHostTest($_GET['ip'],$_GET['tcp'],$_GET['udp'],$_GET['time']))
		echo "Reached=Yes\n";
	else
		echo "Reached=No\n";
/**
 * Parse configuration and call actual host testing.
 * May open a http connection
 *
 * @param string $req Request Data
 */ 
function testHostConn(&$req) {
	global $config;
	$mode = ParseINI::parseValue('hosttest_mode', $config);
	if($mode == 0)
		return true; // Say it works if we didn't test.
	$timeout = ParseINI::parseValue('hosttest_timeout', $config);
	$addr = ParseINI::parseValue('Address', $req);
	preg_match('/TCP:0.0.0.0:([0-9]+),|$/',$addr, $addr_tcp);
	$tcpport = count($addr_tcp)==2 ? ((int) $addr_tcp[1]) : 0; // Be sure that this is injection safe: this machine might have elevated privilegues on the host test provider based on ip
	preg_match('/UDP:0.0.0.0:([0-9]+),|$/',$addr, $addr_udp);
	$udpport = count($addr_udp)==2 ? ((int) $addr_udp[1]) : 0;
	if(!$udpport && !$tcpport)
		return false; // That might be a false false, but it's weird anyway.
	unset($addr, $addr_tcp, $addr_udp);
	if($mode == 1) { // Perform the check from local host
		return performHostTest($_SERVER['REMOTE_ADDR'],$tcpport,$udpport,$timeout);
	} else { // Call hostchecker on remote service
		$url = ParseINI::parseValue('hosttest_url', $config);
		$remotetimeout = ParseINI::parseValue('hosttest_remote_timeout', $config);
		if(!preg_match('#^(([a-z0-9.]+\.[a-z0-9]+)|\[([0-9a-f:]+)\]):([0-9]+)(/.*)$#', $url, $url_split)) {
			trigger_error('Unable to parse hosttest_url from config.', E_USER_WARNING);
			return true; // As earlier
		}
		$remote_start = microtime(true);
		$fp = fsockopen($url_split[1], $url_split[4], $errno, $errstr, $remotetimeout); //fsockopen want's ipv6 with [], http's Host-field however does not!
		if(!$fp) {
			trigger_error('Unable to connect to remote C4HostTest '.$url_split[1].':'.$url_split[4]." in $remotetimeout secons.", E_USER_WARNING);
			return true;
		}
		$req_str = $url_split[5] . '?remotecall&tcp='.$tcpport.'&udp='.$udpport.'&ip='.$_SERVER['REMOTE_ADDR'].'&time='.$timeout;
		$remotetimeout += $timeout;	
		fwrite($fp, "GET ".$req_str." HTTP/1.0\r\nHost: ".($url_split[2]?$url_split[2]:$urlsplit[3])."\r\nUser-Agent: C4Masterserver\r\n\r\n"); // Injection warning!
		$reply = '';
		do {
			stream_set_timeout($fp, $remotetimeout-microtime(true)+$remote_start);
			$reply .= fread($fp, 8192);
		} while (!feof($fp) && $remotetimeout > (microtime(true)-$remote_start));
		if(!preg_match('#^HTTP/1.[01] 200#', $reply)) {			
			trigger_error('Unable to process response from C4HostTest. Wrong address in hosttest_url? No redirects allowed!', E_USER_WARNING);
			return true;
		}
		if(!preg_match('/\r\n\r\n(.*\n)?Reached=[Yy]es\r?\n/s', $reply))
			return false;
		return true;
	}
}
/**
 * Perform host test on $host with $tcpport and $udpport, maximally take $timeout time.
 *
 */
function performHostTest ($host,$tcpport,$udpport,$timeout) {
	global $C4HostTestIncludeMode;
	if($tcpport < 1024) 
		$tcpport = false;
	if($udpport < 1024)
		$udpport = false;
	if($timeout < 0.001) // 0 check
		return false;
	if($timeout > 60)
		$timeout = 60; // Actually, something larger than 30 seconds doesn't make much sense
	$udpreqhex = array( // Magic UDP data stolen from a capture.
		0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
		0x00, 0x02, 0x00, 0x2b, 0x73, 0x3e, 0xb4, 0x6c, 
		0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00 );
	$udpreq = '';
	for($i=0; $i<count($udpreqhex); ++$i) {
		$udpreq .= chr($udpreqhex[$i]);
	}
	$timeend = microtime(true) + $timeout; 
	$udpreply = false;
	while(($cyclestart = microtime(true)) < $timeend) {
		if(!$udp && $udpport)
			if($udp = @fsockopen('udp://'.$host, $udpport, $udperrno, $udperrstr, 0.1)) // Timeout 1 sec, should theretically not take 'time' at all
				stream_set_blocking($udp, false);
		if($udp && !$udpreply) {
			if(microtime(true) - $lastudppack > $timeout/5) // Send 5 Paks a max, usually because tcp waits for half timeout.
				fwrite($udp, $udpreq);
			$lastudppack = microtime(true);
			$udpreply = fread($udp,1500);
		}
		if(!$tcp && $tcpport)
			$tcp = @fsockopen($host, $tcpport, $tcperrno, $tcperrstr, min($timeout/2 - 0.1, $timeend-microtime(true)));
		usleep(max(0,min($timeout*1000000/20,microtime(true)-$cylcestart))); // That means 20 cycles limit, even if $stuff goes wrong
		if ($tcp || $udpreply)
			break;
	}
	if(!$C4HostTestIncludeMode)
		if($tcperrno && !$tcp)
			echo "TCP=Error $tcperrno - $tcperrstr\n";
		else if($tcp) 
			echo "TCP=Reached\n";
		else 
			echo "TCP=Unknown\n"; //Somewhat unbeliegable that UDP replied that quick. 
	if(!$C4HostTestIncludeMode)
		if($udperrno && !$udp)
			echo "UDP=Error $udperrno - $udperrstr\n";
	if(!$udpreply && $udp)
		$udpreply = fread($udp,1500); // Last chance udp!
	if(!$C4HostTestIncludeMode)
		if(!$tcp && !$udpreply) // we can't say if tcp was just faster, so: ignore
			echo "UDP=Timeout\n";
		else if($tcp && !$udpreply)
			echo "UDP=Unknown\n";
		else if($udpreply)
			echo "UDP=Reached\n";
	return ($tcp || $udpreply); // Reachable if a tcp connection could be made, udp replied
}
?>

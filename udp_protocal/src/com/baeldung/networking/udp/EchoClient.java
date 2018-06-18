package com.baeldung.networking.udp;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class EchoClient {
	private DatagramSocket socket;
	private InetAddress address;
	
	private byte[] buf;
	
	public EchoClient() {
		try {
			socket =new DatagramSocket();
			address =InetAddress.getByName("localhost");
		} catch(IOException e) {
			e.printStackTrace();
		}
		
	}
	
	public String sendEcho(String msg) {
		DatagramPacket packet =null;
		try {
			buf =msg.getBytes();
			packet =new DatagramPacket(buf, buf.length, address, 4445);
			System.out.println("this is client, now sending O.o\n\n");
			socket.send(packet);
			String send =new String(packet.getData(), 0, packet.getLength());
			System.out.println(send);
			packet =new DatagramPacket(buf, buf.length);
			socket.receive(packet);
		} catch (IOException e) {
			e.printStackTrace();
		}
		String received =new String(packet.getData(), 0, packet.getLength());
		
		//System.out.println(received);
		return received;
	}
	
	public void close() {
		socket.close();
	}
	
}

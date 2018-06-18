package com.baeldung.networking.udp;

import java.io.IOException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;


public class UDPTest {
	private EchoClient client;
	
	@Before
	public void setup() throws IOException {
        new EchoServer().start();
        client = new EchoClient();
    }
	
	public void whenCanSendAndReceivePacket_thenCorrect() {
        String echo = client.sendEcho("hello server");
        assertEquals("hello server", echo);
        echo = client.sendEcho("server is working");
        assertFalse(echo.equals("hello server"));
    }

	@Test
    public void tearDown() {
        stopEchoServer();
        client.close();
    }
	
	private void stopEchoServer() {
        client.sendEcho("end");
    }
	
}



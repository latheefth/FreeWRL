package sai;
import org.web3d.x3d.sai.*;
import java.util.*;
import java.io.*;
import java.applet.*;
import java.net.*;

public class BrowserFactory {

	private static BrowserFactoryImpl freewrlFactory;
	private static Properties freewrlProperties;
	private static ExternalBrowser b; 
	static {
	
		freewrlProperties = new Properties();

		InputStream is = BrowserFactory.class.getClassLoader().getResourceAsStream("freewrlsai.properties");

		if (is == null) {
			freewrlProperties.put("factory", "sai.FreeWRLFactory");
		} else {
			try {
				freewrlProperties.load(is);
			} catch (IOException e) {
				System.out.println(e);
			}
		}
		b = null;
	}

	
	private BrowserFactory() {

	}

	public static void setBrowserFactoryImpl(BrowserFactoryImpl fac) throws IllegalArgumentException, X3DException, SecurityException {
		if (freewrlFactory != null) {
			throw new X3DException("Factory has already been defined");
		}

		if (fac == null) {
			throw new IllegalArgumentException("Null factory passed to setBrowserFactoryImpl");
		}
	
		freewrlFactory = fac;
	}

	public static X3DComponent createX3DComponent(Map params) throws NotSupportedException {
		if (freewrlFactory == null)
			loadFactory();

		return freewrlFactory.createX3DComponent(params);
	}
	
	//public static ExternalBrowser getBrowser(Applet applet) throws NotSupportedException, NoSuchBrowserException {
	public static ExternalBrowser getBrowser() throws NotSupportedException, NoSuchBrowserException {
		if (freewrlFactory == null)
			loadFactory();

		//ExternalBrowser b = freewrlFactory.getBrowser(applet);
		//ExternalBrowser b = freewrlFactory.getBrowser();
		if(b == null)
			b = freewrlFactory.getBrowser();

		if (b == null) {
			throw new NoSuchBrowserException("getBrowser(): no such browser found");
		}

		return b;
	}

	public static ExternalBrowser getBrowser(Applet applet, String frameName, int index) throws NotSupportedException, NoSuchBrowserException {
		if (freewrlFactory == null)
			loadFactory();

                //ExternalBrowser b = freewrlFactory.getBrowser(applet);
                //ExternalBrowser b = freewrlFactory.getBrowser();
				if(b == null)
					b = freewrlFactory.getBrowser();

                if (b == null) {
                        throw new NoSuchBrowserException("getBrowser(Applet, String, int): no such browser found");
                }

                return b;

	}

	public static ExternalBrowser getBrowser(InetAddress address, int port) throws NotSupportedException, NoSuchBrowserException, UnknownHostException, ConnectionException {
		if (freewrlFactory == null)
			loadFactory();
		if(b == null)
			b = freewrlFactory.getBrowser(address, port);
		return b;
	}

	private static void loadFactory() {
		try {
			String factoryClassName = (String) freewrlProperties.getProperty("factory");
			Class  factoryClass = Class.forName(factoryClassName);
			freewrlFactory = (BrowserFactoryImpl)factoryClass.newInstance(); 
		} catch (Exception e) {
			System.out.println(e);
		}
	}
}

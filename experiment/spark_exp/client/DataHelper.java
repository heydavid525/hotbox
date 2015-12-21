package exp.spark;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.apache.spark.mllib.linalg.SparseVector;
import org.apache.spark.mllib.linalg.Vector;
import org.apache.spark.mllib.regression.LabeledPoint;

public class DataHelper {
	static final int NUM_SLICE = 10000;
	
	static final int NUM_DATA = 2396130;
	
	public static List<LabeledPoint> getData(long data_begin, long data_end) {
		Socket socket = null;
		try {
			socket = new Socket("localhost", 13579);
			OutputStream os = socket.getOutputStream();
			DataOutputStream dos = new DataOutputStream(os);
			InputStream is = socket.getInputStream();
			DataInputStream dis = new DataInputStream(is);

			//createSession(dis, dos);			
			List<LabeledPoint> dataList = getDataList(dis, dos, data_begin, data_end);
			dos.close();
			dis.close();
			System.out.println("datalist len: " + dataList.size());
			return dataList;
		} catch (Exception e) {
			e.printStackTrace();
			return new ArrayList<LabeledPoint>();
		}
	}
	
	private static void createSession(DataInputStream dis, DataOutputStream dos) throws Exception{
		String body = "url_combined test_session /home/wanghy/github/hotbox/test/resource/select_all.conf false";
		byte[] header = createHeader((byte)0x01, body.getBytes().length);
        dos.write(header, 0, 5);
		dos.write(body.getBytes());
		dos.flush();
		byte[] res = new byte[1];
		dis.read(res, 0, 1);
		System.out.printf("create res: %x\n", res[0]);
	}
	
	private static List<LabeledPoint> getDataList(DataInputStream dis, DataOutputStream dos, long data_begin, long data_end) throws Exception{
		String body = data_begin + " " + data_end;
		List<LabeledPoint> dataList = new ArrayList<LabeledPoint>();
		byte[] header = createHeader((byte)0x02, body.getBytes().length);
		dos.write(header, 0, 5);
		dos.write(body.getBytes());
		dos.flush();
		for(int slice = 1; slice <= NUM_SLICE; slice++){
			int resLen = dis.readInt();
			//System.out.println("data resLen:" + resLen);
			byte[] dataRes = new byte[resLen];
			dis.readFully(dataRes, 0, resLen);
			DataInputStream bdis = new DataInputStream(new ByteArrayInputStream(dataRes));
			while(true){
				try{
					int count = bdis.readInt();
					byte[] datum = new byte[count];
					bdis.read(datum, 0, count);
					DataInputStream datumDis = new DataInputStream(new ByteArrayInputStream(datum));
					long dim = datumDis.readLong();
					float label = datumDis.readFloat();
					if(label == -1)
						label = 0;
					List<Integer> indicesList = new ArrayList<Integer>();
					List<Double> valuesList = new ArrayList<Double>();
					while(true){
						try{
							indicesList.add(datumDis.readInt() - 1);
							valuesList.add((double)datumDis.readFloat());
						}catch(EOFException e){
							datumDis.close();
							break;
						}
					}	
					int[] indices = new int[indicesList.size()];
					double[] values = new double[valuesList.size()];
					for (int i = 0; i < indicesList.size(); i++)
						indices[i] = indicesList.get(i);
					for (int i = 0; i < valuesList.size(); i++)
						values[i] = valuesList.get(i);
					Vector features = new SparseVector((int) dim, indices, values);
					dataList.add(new LabeledPoint(label, features));
				}catch(EOFException e){
					bdis.close();
					break;
				}					
			}
			//System.out.println("datalist len " + dataList.size());
		}
		return dataList;
	}
	
	private static byte[] createHeader(byte type, int length){
		byte[] header = new byte[5];
		header[0] = type;
		header[1] = (byte) (length >> 24);  
		header[2] = (byte) (length >> 16);  
		header[3] = (byte) (length >> 8);  
		header[4] = (byte) length; 
		return header;
	}
}

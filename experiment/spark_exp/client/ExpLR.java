package exp.spark;

import scala.Tuple2;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.Function;
import org.apache.spark.api.java.function.VoidFunction;
import org.apache.spark.mllib.classification.LogisticRegressionModel;
import org.apache.spark.mllib.classification.LogisticRegressionWithLBFGS;
import org.apache.spark.mllib.evaluation.MulticlassMetrics;
import org.apache.spark.mllib.linalg.SparseVector;
import org.apache.spark.mllib.linalg.Vector;
import org.apache.spark.mllib.regression.LabeledPoint;
import org.apache.spark.mllib.util.MLUtils;
import org.apache.spark.SparkConf;
import org.apache.spark.SparkContext;

import java.sql.SQLException;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.Statement;
import java.sql.DriverManager;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.apache.spark.sql.DataFrame;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.hive.*;

import com.sun.xml.bind.v2.runtime.unmarshaller.XsiNilLoader.Array;

public class ExpLR {
	
  static List<String> datalist;
  
  public static void main(String[] args) {
    SparkConf conf = new SparkConf().setAppName("LR on kddb");
    conf.set("spark.network.timeout", "1200000");
    conf.set("spark.driver.maxResultSize", "30g");
    long start = 0, end = 0;
    SparkContext sc = new SparkContext(conf);
    JavaSparkContext jsc = new JavaSparkContext(sc);
    
    getData();
    JavaRDD<String> inputData = jsc.parallelize(datalist);
    start = System.currentTimeMillis();
    JavaRDD<LabeledPoint> training = inputData.map(new Function<String, LabeledPoint>() {
	  public LabeledPoint call(String row) {
		  String record = row;
		  String[] eles = record.split(" ");
		  long dim = Long.parseLong(eles[0]);
		  double lable = Double.parseDouble(eles[1]);
		  
		  List<Integer> indicesList = new ArrayList<Integer>();
		  List<Double> valuesList = new ArrayList<Double>();
		  for(int i = 2; i < eles.length; i++){
			  String[] feature = eles[i].split(":");
			  indicesList.add(Integer.parseInt(feature[0])-1);
			  valuesList.add(Double.parseDouble(feature[1]));
		  }
		  int[] indices = new int[indicesList.size()];
		  double[] values = new double[valuesList.size()];
		  for(int i = 0; i < indicesList.size(); i++)
			  indices[i] = indicesList.get(i);
		  for(int i = 0; i < valuesList.size(); i++)
			  values[i] = valuesList.get(i);
		  Vector features = new SparseVector((int)dim, indices, values); 
		  return new LabeledPoint(lable, features);
	  }
	});
    training.count();
    end = System.currentTimeMillis();
    System.out.println("transform: " + (end - start));
    start = System.currentTimeMillis();
    training.count();
    end = System.currentTimeMillis();
    System.out.println("second count: " + (end - start));
    // Run training algorithm to build the model.
    start = System.currentTimeMillis();
    final LogisticRegressionModel model = new LogisticRegressionWithLBFGS()
      .setNumClasses(2)
      .run(training.rdd());
    end = System.currentTimeMillis();
    System.out.println("training: " + (end - start));
  }
  
  public static void getData() {
      Socket socket = null;
      try {
          socket = new Socket("10.1.1.21", 13579);
          OutputStream os = socket.getOutputStream();
          DataOutputStream dos = new DataOutputStream(os);
          InputStream is = socket.getInputStream();
          DataInputStream dis = new DataInputStream(is);
          String body = "test_db test_session /home/wanghy/github/hotbox/test/resource/test_transform1.conf true";
          byte[] header = new byte[1];
          header[0] = 0x01;          
          
          dos.write(header);
          dos.writeInt(body.getBytes().length);
          System.out.println("body len:" + body.getBytes().length);
          dos.write(body.getBytes());
          
          byte[] res = new byte[1];
          dis.read(res, 0, 1);
          System.out.printf("create res: %x\n", res[0]);
          
          header[0] = 0x02;
          body = "0 5";
          
          dos.write(header);   
          dos.writeInt(body.getBytes().length);
          dos.write(body.getBytes());
          
          int resLen = dis.readInt();
          System.out.println("data resLen:" + resLen);
          byte[] dataRes = new byte[1024*1024];
          dis.read(dataRes, 0, resLen);
          datalist = new ArrayList(Arrays.asList(new String(dataRes).split(";;;")));
          datalist.remove((datalist.size() - 1));
          System.out.println(datalist);
          
          dos.close();
          dis.close();
      } catch (Exception e) {
          e.printStackTrace();
      } 
  }
  
  public static byte[] getBytes(int data)
  {
      byte[] bytes = new byte[4];
      bytes[0] = (byte) (data & 0xff);
      bytes[1] = (byte) ((data & 0xff00) >> 8);
      bytes[2] = (byte) ((data & 0xff0000) >> 16);
      bytes[3] = (byte) ((data & 0xff000000) >> 24);
      return bytes;
  }
}
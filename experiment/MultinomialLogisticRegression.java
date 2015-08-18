package test.lr.spark;

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
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.Statement;
import java.sql.DriverManager;
import java.util.ArrayList;
import java.util.List;

public class MultinomialLogisticRegression {
  private static String driverName = "org.apache.hive.jdbc.HiveDriver";
  
  public static void main(String[] args) {
    SparkConf conf = new SparkConf().setAppName("LR on url_combined");
    JavaSparkContext jsc = new JavaSparkContext(conf);
//    SparkContext sc = new SparkContext(conf);
    long start = 0, end = 0;
    List<LabeledPoint> dataList = new ArrayList<LabeledPoint>();
    try {
      Class.forName(driverName);
      //replace "hive" here with the name of the user the queries should run as
      Connection con = DriverManager.getConnection("jdbc:hive2://localhost:10000/default", "hive", "123456");
      Statement stmt = con.createStatement();
      String sql = "select * from url_combined limit 2396130";
      System.out.println("Running: " + sql);
      start = System.currentTimeMillis();
      ResultSet res = stmt.executeQuery(sql);
      end = System.currentTimeMillis();
      System.out.println("hive load data: " + (end - start));
      start = System.currentTimeMillis();
      while (res.next()) {
    	  String record = res.getString(1);
    	  String[] eles = record.split(" ");
    	  double lable = Double.parseDouble(eles[0]);
    	  List<Integer> indicesList = new ArrayList<Integer>();
    	  List<Double> valuesList = new ArrayList<Double>();
    	  for(int i = 1; i < eles.length; i++){
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
    	  Vector features = new SparseVector(3231961, indices, values);
    	  
    	  LabeledPoint parsedRecord = new LabeledPoint(lable, features);
    	  dataList.add(parsedRecord);
      }
    } catch (Exception e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
        System.exit(1);
    }    
    
//    String path = "/url_combined";
//    JavaRDD<LabeledPoint> training = MLUtils.loadLibSVMFile(sc, path).toJavaRDD();
    JavaRDD<LabeledPoint> training = jsc.parallelize(dataList);
    end = System.currentTimeMillis();
    System.out.println("parse to rdd: " + (end - start));
    start = System.currentTimeMillis();
    training = training.map(
    		new Function<LabeledPoint, LabeledPoint>() {
				@Override
				public LabeledPoint call(LabeledPoint x) throws Exception {
					double label = 1;     		
					if(x.label() == -1) 
						label = 0; 
					return new LabeledPoint(label,x.features());
				}
			}
    	);
    training.count();
    end = System.currentTimeMillis();
    System.out.println("transform: " + (end - start));

    // Run training algorithm to build the model.
    start = System.currentTimeMillis();
    final LogisticRegressionModel model = new LogisticRegressionWithLBFGS()
      .setNumClasses(2)
      .run(training.rdd());
    end = System.currentTimeMillis();
    System.out.println("training: " + (end - start));
  }
}
package exp.spark;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.FlatMapFunction;
import org.apache.spark.api.java.function.Function;
import org.apache.spark.api.java.function.Function2;
import org.apache.spark.mllib.classification.LogisticRegressionModel;
import org.apache.spark.mllib.classification.LogisticRegressionWithLBFGS;
import org.apache.spark.mllib.linalg.SparseVector;
import org.apache.spark.mllib.linalg.Vector;
import org.apache.spark.mllib.regression.LabeledPoint;

import scala.runtime.SeqCharSequence;

import org.apache.spark.SparkConf;
import org.apache.spark.SparkContext;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

public class ExpLR {
	static final int NUM_DATA = 2396130;
	static final int NUM_PARTITION = 16;
	
	public static void main(String[] args) {
		SparkConf conf = new SparkConf().setAppName("LR on hotbox");
		conf.set("spark.network.timeout", "1200000");
		conf.set("spark.driver.maxResultSize", "30g");
		conf.set("spark.eventLog.enabled", "true");
		conf.set("spark.eventLog.dir", "/home/wanghy");
		long start = 0, end = 0;
		SparkContext sc = new SparkContext(conf);
		JavaSparkContext jsc = new JavaSparkContext(sc);
		
		List<Integer> partitions = new ArrayList<>();
		for(int i = 0; i < NUM_PARTITION; i++)
			partitions.add(i);
		start = System.currentTimeMillis();
		JavaRDD<LabeledPoint> inputData = jsc.parallelize(partitions).repartition(NUM_PARTITION)
				.mapPartitionsWithIndex(new Function2<Integer, Iterator<Integer>, Iterator<LabeledPoint>>() {
			@Override
			public Iterator<LabeledPoint> call(Integer index, Iterator<Integer> id) throws Exception {
				int slice_len = NUM_DATA / NUM_PARTITION;
				System.out.println("index: " + index);
				if(index < NUM_PARTITION - 1)
					return DataHelper.getData(index * slice_len, (index + 1) * slice_len).iterator();
				else
					return DataHelper.getData((NUM_PARTITION - 1) * slice_len, -1).iterator();
			}
		}, true);
		
		inputData.cache();
		end = System.currentTimeMillis();
		System.out.println("getdata: " + (end - start));
		
		start = System.currentTimeMillis();
		System.out.println("num data: " + inputData.count());
		end = System.currentTimeMillis();
		System.out.println("count: " + (end - start));
		// Run training algorithm to build the model.
		start = System.currentTimeMillis();
		final LogisticRegressionModel model = new LogisticRegressionWithLBFGS().setNumClasses(2).run(inputData.rdd());
		end = System.currentTimeMillis();
		System.out.println("training: " + (end - start));
	}
}
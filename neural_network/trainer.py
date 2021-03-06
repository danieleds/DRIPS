import tensorflow as tf
import numpy as np
import pickle
import random

input_size = 30
output_size = 8 # num of possible road configurations

# input: [...FFT0][...FFT1][...FFT2]
# output:
#
#   000 -> 1,0,0,0,0,0,0,0  (no cars)
#   001 -> 0,1,0,0,0,0,0,0  (just one car on the right)
#   010 -> 0,0,1,0,0,0,0,0  (just one car in front of us)
#   011 -> 0,0,0,1,0,0,0,0  (one car in front of us, and one on the right)
#   100 -> 0,0,0,0,1,0,0,0  (just one car on the left)
#   101 -> 0,0,0,0,0,1,0,0  ...
#   110 -> 0,0,0,0,0,0,1,0  ...
#   111 -> 0,0,0,0,0,0,0,1  ...

x = tf.placeholder(tf.float32, [None, input_size])

# === BEGIN DEEP NN ===

# hidden layer
#num_nodes = 20
#weights_hidden = tf.Variable(tf.random_normal([input_size, num_nodes]))
#bias_hidden = tf.Variable(tf.random_normal([num_nodes]))
#preactivations_hidden = tf.add(tf.matmul(x, weights_hidden), bias_hidden)
#activations_hidden = tf.nn.relu(preactivations_hidden)

# hidden layer
#num_nodes_2 = 50
#weights_hidden_2 = tf.Variable(tf.random_normal([num_nodes, num_nodes_2]))
#bias_hidden_2 = tf.Variable(tf.random_normal([num_nodes_2]))
#preactivations_hidden_2 = tf.add(tf.matmul(activations_hidden, weights_hidden_2), bias_hidden_2)
#activations_hidden_2 = tf.nn.sigmoid(preactivations_hidden_2)

# output layer
#weights_output = tf.Variable(tf.random_normal([num_nodes, output_size]))
#bias_output = tf.Variable(tf.random_normal([output_size]))
#preactivations_output = tf.add(tf.matmul(activations_hidden, weights_output), bias_output)

#y = tf.nn.relu(preactivations_output)

# === END DEEP NN ===


# === BEGIN SIMPLE NN ===

#W = tf.Variable(tf.zeros([input_size, output_size]))
W = tf.Variable(tf.truncated_normal([input_size, output_size], stddev=1./22.))
b = tf.Variable(tf.zeros([output_size]))
y = tf.nn.softmax(tf.matmul(x, W) + b)

# === END SIMPLE NN ===


# ===== Training =====

with open('data.pickle', 'rb') as f:
  data = pickle.load(f)
  random.shuffle(data)
  train_data = [v[0] for v in data]
  train_labels = [v[1] for v in data]

y_ = tf.placeholder(tf.float32, [None, output_size])
cross_entropy = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y_, logits=y))

train_step = tf.train.GradientDescentOptimizer(0.5).minimize(cross_entropy)


sess = tf.InteractiveSession()

tf.global_variables_initializer().run()

train_data_ratio = 0.666 # Percentage of train data over total data
_max = int(len(train_data) * train_data_ratio)
step = 5
for i in range(0, _max, step):
  batch_xs, batch_ys = train_data[i:i+step], train_labels[i:i+step]
  sess.run(train_step, feed_dict={x: batch_xs, y_: batch_ys})

correct_prediction = tf.equal(tf.argmax(y,1), tf.argmax(y_,1))
accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))
accuracy_val = sess.run(accuracy, feed_dict={x: train_data[_max-len(train_data):], y_: train_labels[_max-len(train_data):]})
print(str(i+step) + " / " + str(_max) + "; accuracy: " + str(accuracy_val))

# Save weights
W_val, b_val = sess.run([W, b])
np.savetxt("W.csv", W_val, delimiter=",")
np.savetxt("W'.csv", np.array(W_val).transpose(), delimiter=",")
np.savetxt("b.csv", np.array(b_val).transpose(), delimiter=",")

# Save C snippet
out_file = open("snippet.c", "w")
out_file.write("static const float W[OUTPUT_SIZE][INPUT_SIZE] PROGMEM = {\n")
for val in np.array(W_val).transpose():
  out_file.write('    {')
  out_file.write(','.join(['{:.18e}'.format(x) for x in val]))
  out_file.write("},\n")
out_file.write("};\n\n")
out_file.write('static const float b[OUTPUT_SIZE] PROGMEM = { ')
out_file.write(','.join(['{:.18e}'.format(x) for x in b_val]))
out_file.write(" };\n")

#out_file.write("This Text is going to out file\nLook at it and see\n")
out_file.close()
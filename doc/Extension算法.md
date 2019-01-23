# DNN

In order to learn the kind of complicated functions that can represent high-level abstractions (e.g. in vision, language, and other AI-level tasks), one needs deep architectures. [LDA]

A deep neural network (DNN) is an artificial neural network with multiple layers between the input and output layers. [Wiki-DNN]

Deep convolutional neural networks have led to a series of breakthroughs for image classification. [DRL],[ImageNet with DCNN]

the network depth is of crucial importance, and the leading results on the challenging ImageNet dataset all exploit “very deep” models, with a depth of sixteen to thirty. [DRL] 

GoogLeNet是一个22层的深度网络，它的理念是没有最深，只有更深。它增加了网络的宽度和深度，在分类和检测任务上均取得了不错的效果。GoogLeNet采用了Inception的概念，利用网中网（Network in Network）的结构[NIN]，将原来的每个节点都用一个网络来表示，使得整个网络结构的深度更深，更能挖掘出更有特点的属性。[GoogLeNet]

Deep Residual Network(Deep ResNet)也是一个深度网络，现在最深可以达到千层。它主要是为了解决在网络加深后，准确度快速退化的问题。（with the network depth increasing, accuracy gets saturated (which might be unsurprising) and then degrades rapidly.）[DRL]

# Motivation

深层神经网络同样可以被抽象成一个有向无环图。它相比于一般的神经网络，不同之处在于它具有更长的拓扑结构。通过比较，常见的一般网络层数不深，如LeNet仅有5层，AlexNet仅有8层，VGG有19层等，而深度神经网络的层数会比这些网络多上好几倍，如ResNet最初提出时就已经达到了152层[DRL]，现在更是发展到了上千层。

对于这些深度神经网络的训练或预测过程是比较缓慢的，有些网络虽然在提出时就考虑到了计算性能的问题，但由于网络的深度及宽度，仍然需要耗费很多的计算资源。

# Algorithm



# Reference

1. [LDA] Learning Deep Architectures for AI
2. [Wiki-DNN] https://en.wikipedia.org/wiki/Deep_learning#Deep_neural_networks
3. [DRL] He, Kaiming, et al. "Deep Residual Learning for Image Recognition."*computer vision and pattern recognition* (2016): 770-778.
4. [ImageNet with DCNN] Krizhevsky A, Sutskever I, Hinton G E, et al. ImageNet Classification with Deep Convolutional Neural Networks[C]. neural information processing systems, 2012: 1097-1105.
5. [GoogLeNet] Szegedy C, Liu W, Jia Y, et al. Going deeper with convolutions[J]. computer vision and pattern recognition, 2015: 1-9.
6. [NIN] Min Lin, Qiang Chen, and Shuicheng Yan. Network in network. CoRR, abs/1312.4400, 2013


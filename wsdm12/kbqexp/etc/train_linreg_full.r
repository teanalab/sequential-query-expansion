train_data<-read.table("/home/alex/projects/kbqexp/results/wt2g/rank/rank-new-3/splits/train.5", header=F);
names(train_data)<-c("qId","expCand","numQryTerms","topDocScore","expTDocScore","topTermFrac","numCanDocs","avgCDocScore","maxCDocScore","idf","fanOut","spActScore","spActRank","rndWalkScore","pathFindScore","avgColCor","maxColCor","avgTopCor","maxTopCor","avgTopPCor","maxTopPCor","avgQDist","maxQDist","avgPWeight","maxPWeight","map");
attach(train_data);
log_reg<-glm(map~numQryTerms+topDocScore+expTDocScore+topTermFrac+numCanDocs+avgCDocScore+maxCDocScore+idf+fanOut+spActScore+spActRank+rndWalkScore+pathFindScore+avgColCor+maxColCor+avgTopCor+maxTopCor+avgTopPCor+maxTopPCor+avgQDist+maxQDist+avgPWeight+maxPWeight);
log_reg
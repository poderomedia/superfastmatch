#include <tests.h>

typedef BaseTest AssociationTest;

TEST_F(AssociationTest,ShortTest){
  EXPECT_CALL(registry_,getWindowSize())
    .WillRepeatedly(Return(4));
  DocumentPtr doc1 = registry_.getDocumentManager()->createPermanentDocument(1,1,"text=This1");
  DocumentPtr doc2 = registry_.getDocumentManager()->createPermanentDocument(1,2,"text=This2");
  Association* association = new Association(&registry_,doc1,doc2);
  EXPECT_EQ(1U,association->getResultCount());
  EXPECT_EQ(4U,association->getLength(0));
  EXPECT_STREQ("This",association->getToResult(0).c_str());
}

TEST_F(AssociationTest,ConstructorTest){
  DocumentPtr doc1 = registry_.getDocumentManager()->createPermanentDocument(1,1,"text=This+is+a+long+sentence+where+the+phrase+Always+Look+On+The+Bright+Side+Of+Life&title=Doc1");
  DocumentPtr doc2 = registry_.getDocumentManager()->createPermanentDocument(1,2,"text=Always+Look+On+The+Bright+Side+Of+Life+and+this+is+a+long+sentence&title=Doc2");
  EXPECT_STREQ("This is a long sentence where the phrase Always Look On The Bright Side Of Life",doc1->getText().c_str());
  EXPECT_STREQ("Always Look On The Bright Side Of Life and this is a long sentence",doc2->getText().c_str());
  Association* association = new Association(&registry_,doc1,doc2);
  EXPECT_EQ(2U,association->getResultCount());
  EXPECT_EQ(38U,association->getLength(0));
  EXPECT_STREQ("Always Look On The Bright Side Of Life",association->getToResult(0).c_str());
  EXPECT_STREQ(association->getToResult(0).c_str(),association->getFromResult(0).c_str());
  EXPECT_EQ(23U,association->getLength(1));
  EXPECT_STREQ("This is a long sentence",association->getFromResult(1).c_str());
  EXPECT_STRCASEEQ(association->getToResult(1).c_str(),association->getFromResult(1).c_str());
  EXPECT_EQ(61U,association->getTotalLength());
  EXPECT_EQ(2U,registry_.getDocumentDB()->count());
  EXPECT_EQ(0U,registry_.getAssociationDB()->count());
  EXPECT_EQ(true,association->save());
  EXPECT_EQ(2U,registry_.getDocumentDB()->count());
  EXPECT_EQ(2U,registry_.getAssociationDB()->count());
  Association* association2 = new Association(&registry_,doc1,doc2);
  EXPECT_EQ(2U,association2->getResultCount());  
  Association* association3 = new Association(&registry_,doc2,doc1);
  EXPECT_EQ(2U,association3->getResultCount());
  EXPECT_STRCASEEQ(association2->getFromResult(1).c_str(),association3->getFromResult(1).c_str());
  EXPECT_EQ(2U,registry_.getAssociationDB()->count());
}

TEST_F(AssociationTest, WhitespaceTest){
  DocumentPtr doc1 = registry_.getDocumentManager()->createTemporaryDocument("text=++++++++++++++++++++whitespace+test+with+a+long+sentence+*+*+*+*+*+*+*+*+*+*+*+*+");
  DocumentPtr doc2 = registry_.getDocumentManager()->createTemporaryDocument("text=++++++++++++++++++++whitespace+test+with+a+long+sentence+*+*+*+*+*+*+*+*+*+*+*+*+");
  Association* association = new Association(&registry_,doc1,doc2);
  EXPECT_STREQ("whitespace test with a long sentence",association->getToResult(0).c_str());
}

TEST_F(AssociationTest, EndingTest){
  DocumentPtr doc1 = registry_.getDocumentManager()->createTemporaryDocument("text=This+is+Captain+Franklin");
  DocumentPtr doc2 = registry_.getDocumentManager()->createTemporaryDocument("text=This+is+Captain+Francis");
  Association* association = new Association(&registry_,doc1,doc2);
  EXPECT_STREQ("This is Captain Fran",association->getToResult(0).c_str());
}

TEST_F(AssociationTest, ManagerPermanentTest){
  DocumentPtr doc1 = registry_.getDocumentManager()->createPermanentDocument(1,1,"text=This+is+a+long+sentence+where+the+phrase+Always+Look+On+The+Bright+Side+Of+Life&title=Doc1");
  DocumentPtr doc2 = registry_.getDocumentManager()->createPermanentDocument(1,2,"text=Always+Look+On+The+Bright+Side+Of+Life+and+this+is+a+long+sentence&title=Doc2");
  registry_.getPostings()->addDocument(doc1);
  registry_.getPostings()->addDocument(doc2);
  registry_.getPostings()->finishTasks();
  EXPECT_NE(0U,registry_.getPostings()->getHashCount());
  SearchPtr search=Search::createPermanentSearch(&registry_,doc1->doctype(),doc1->docid());
  EXPECT_EQ(1U,search->associations.size());
  EXPECT_EQ(2U,registry_.getAssociationDB()->count());
  DocumentPtr doc3 = registry_.getDocumentManager()->getDocument(1,1);
  EXPECT_NE(0U,doc3->getText().size());
  vector<AssociationPtr> savedAssociations = registry_.getAssociationManager()->getAssociations(doc3,DocumentManager::NONE);
  EXPECT_EQ(1U,savedAssociations.size());
  EXPECT_TRUE(registry_.getAssociationManager()->removeAssociations(doc3));
  EXPECT_EQ(0U,registry_.getAssociationDB()->count());
}

TEST_F(AssociationTest, ManagerTemporaryTest){
  DocumentPtr doc1 = registry_.getDocumentManager()->createPermanentDocument(1,1,"text=This+is+a+long+sentence+where+the+phrase+Always+Look+On+The+Bright+Side+Of+Life&title=Doc1");
  registry_.getPostings()->addDocument(doc1);
  registry_.getPostings()->finishTasks();
  EXPECT_NE(0U,registry_.getPostings()->getHashCount());
  SearchPtr search=Search::createTemporarySearch(&registry_,"text=Always+Look+On+The+Bright+Side+Of+Life+and+this+is+a+long+sentence&title=Doc2");
  EXPECT_EQ(1U,search->associations.size());
  EXPECT_EQ(0U,registry_.getAssociationDB()->count());
}

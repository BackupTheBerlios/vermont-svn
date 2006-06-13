#!/bin/sh

if [ `id -u` -ne 0 ] ; then
	echo "Installation must be started as root. Exiting."
	exit -1
fi

JAVA_HOME=`find /usr -type d -maxdepth 3 -name "j2sdk*" 2> /dev/null | head -1 `

if [ "$JAVA_HOME" = "" ] ; then
	echo -n "Couldn't find Java SDK. Enter correct path: "
	read JAVA_HOME

	if [ ! -d $JAVA_HOME ] ; then
		echo "Path is incorrect. Exiting."
		exit -1
	fi
fi

if [ ! -f $JAVA_HOME/bin/javac ] ; then
	echo "Java compiler could not be found. Exiting."
	exit -1
fi

mkdir -p bin
mkdir -p tomcat_content/WEB-INF/classes
mkdir -p tomcat_content/WEB-INF/lib

$JAVA_HOME/bin/javac -classpath jsp-api.jar:servlet-api.jar:lib/jCharts-0.7.5.jar:lib/itext-1.2.jar -d bin `find src/ -name "*.java"`

if [ "$?" -ne "0" ] ; then
	echo "An error occured during compilation. Exiting."
	exit -1
fi

cd bin

cp --parents de/japes/servlets/nasty/*.class ../tomcat_content/WEB-INF/classes
cp --parents de/japes/parser/*.class ../tomcat_content/WEB-INF/classes
cp --parents de/japes/text/*.class ../tomcat_content/WEB-INF/classes
cp --parents de/japes/beans/nasty/*.class ../tomcat_content/WEB-INF/classes

mkdir /usr/local/nastyCollector
cp --parents de/japes/net/nasty/collector/*.class /usr/local/nastyCollector
cp --parents de/japes/parser/*.class /usr/local/nastyCollector
cp --parents de/japes/parser/nasty/*.class /usr/local/nastyCollector

cd ..

cp collector.cfg /usr/local/nastyCollector/
cp nastyColl /usr/local/bin

# copy standard taglib jar files:
# see also: http://jakarta.apache.org/taglibs/doc/standard-1.0-doc/GettingStarted.html
cp lib/jstl.jar tomcat_content/WEB-INF/lib
cp lib/standard.jar tomcat_content/WEB-INF/lib
cp lib/jaxen-full.jar tomcat_content/WEB-INF/lib
cp lib/saxpath.jar tomcat_content/WEB-INF/lib
cp lib/jdbc2_0-stdext.jar tomcat_content/WEB-INF/lib
cp lib/xercesImpl.jar tomcat_content/WEB-INF/lib
cp lib/xalan.jar tomcat_content/WEB-INF/lib
cp lib/xml-apis.jar tomcat_content/WEB-INF/lib

cp lib/mysql-connector-java-3.0.16-ga-bin.jar /usr/local/nastyCollector/
cp lib/itext-1.2.jar tomcat_content/WEB-INF/lib
cp lib/jCharts-0.7.5.jar tomcat_content/WEB-INF/lib

cd tomcat_content

$JAVA_HOME/bin/jar cvf nasty.war ./*

if [ "$?" -ne "0" ] ; then
        echo "An error occured while creating Web Application.  Exiting."
        exit -1
fi

mv nasty.war ..
cd ..

echo;echo
echo "************************************************************************"
echo "Now copy the nasty.war file to \$TOMCAT_HOME/webapps and restart Tomcat."
echo "The data collector was copied to /usr/local/nastyCollector and can be "
echo "started via a script called nastyColl placed in /usr/local/bin."
echo "************************************************************************"
echo;echo


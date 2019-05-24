#define _GLIBCXX_USE_CXX11_ABI 0

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "AbstractSocket.h"

#include <fstream>
#include <exception>
#include <iostream>

#define VERSION 0.01
class v4l2_exception : public std::exception {
	const char * msg;
	public:
	v4l2_exception(const char * _msg) : msg(_msg) {}
	const char * what() const noexcept {
		return msg;
	}
};

void ioctl_exception(int descriptor, int property, void* structure, char* msg) { 
	if(ioctl(descriptor, property, structure) < 0) {
		throw v4l2_exception(msg);
	}
}

struct videoBuffer {
	v4l2_buffer bufferInfo;
	void* start;
};

struct cameraHeader {
	int cameraID;
	int frameWidth;
	int frameHeight;
};

int main(int argc, char** argv) {
	std::cout << "Initializing Camera Client v." << VERSION << "..." << std::endl;
	if(argc < 7) {
		std::cerr << "Incorrect Arguments. Arguments required: 1. Server Address 2. Port 3. Camera Width 4. Camera Height 5. Camera ID 6. Maximum Reconnect Attempts" << std::endl;
		return 1;
	}
	
	// Stream Parameters Setup

	const char* serverAddress = argv[1];
	int port = atoi(argv[2]);
	int width = atoi(argv[3]);
	int height = atoi(argv[4]);
	int id = atoi(argv[5]);
	int maxReconnect = atoi(argv[6]);

	std::cout << "Addr: " << serverAddress << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Width: " << width << std::endl;
	std::cout << "Height: " << height << std::endl;
	std::cout << "Id: " << id << std::endl;
	std::cout << "Reconnect Attempts: " << maxReconnect << std::endl;

	
	int descriptor = open("/dev/video0", O_RDWR);
	if(descriptor < 0) {
		std::cerr << "Fatal Exception: Unable to open /dev/video0" << std::endl;
		return 1;
	}

	// Initializing Camera and Buffers (Fatal if exception thrown)
	struct videoBuffer buffer;
	memset(&buffer, 0, sizeof(buffer));

	try {
		struct v4l2_capability cap;
		ioctl_exception(descriptor, VIDIOC_QUERYCAP, &cap, (char *)"Failed to query."); // get device capabilities
		
		if(!cap.capabilities & V4L2_CAP_STREAMING) throw v4l2_exception((char *)"This device does not support video streaming");
		
		if(!cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) throw v4l2_exception((char *)"This device does not support single planar capture");
		
		struct v4l2_input input;
		memset(&input, 0, sizeof(input));
		int index = 0;
		
		ioctl_exception(descriptor, VIDIOC_S_INPUT, &index, (char *)"Failed to set input."); // set input index, 0 is default
		
		struct v4l2_format format;
		memset(&format, 0, sizeof(format));
		
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // Set format, currently 1920x1080 H.264 encoded
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;
		format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;

		ioctl_exception(descriptor, VIDIOC_S_FMT, &format, (char *)"Failed to set format");
		
		struct v4l2_streamparm param;
		memset(&param, 0, sizeof(param));
		param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
		param.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
		param.parm.capture.timeperframe.numerator = 1;
		param.parm.capture.timeperframe.denominator = 15;

		ioctl_exception(descriptor, VIDIOC_S_PARM, &param, (char *)"Could not set framerate");
	
		struct v4l2_requestbuffers reqbuf;
		memset(&reqbuf, 0, sizeof(reqbuf));
		
		reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		reqbuf.memory = V4L2_MEMORY_MMAP;
		reqbuf.count = 1;
		
		ioctl_exception(descriptor, VIDIOC_REQBUFS, &reqbuf, (char *)"Could not request buffers");
				
		// for each buffer, map it to userspace and queue it so that the driver can use it 
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = 0;
		
		ioctl_exception(descriptor, VIDIOC_QUERYBUF, &buf, (char *)"Failed to query buffers.");

		void* start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, buf.m.offset);
		
		if(start == MAP_FAILED) throw v4l2_exception((char *)"mmap failed to map buffers.");

		buffer = {buf, start};

		memset(buffer.start, 0, buffer.bufferInfo.length);
		ioctl_exception(descriptor, VIDIOC_QBUF, &buffer.bufferInfo, (char *)"Failed to initially queue buffers");

		int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl_exception(descriptor, VIDIOC_STREAMON, &type, (char *)"Failed to enable streaming.");

	} catch(std::exception& e) {
		std::cout << "Fatal Exception: " << e.what() << std::endl;
		return 1;
	}

	
	/*try {
		
		if(descriptor < 0) throw v4l2_exception((char *)"Could not open video0.");
		
		struct cameraHeader cameraInfo = {CAM_ID, CAM_WIDTH, CAM_HEIGHT};
		
		struct v4l2_capability cap;
		ioctl_exception(descriptor, VIDIOC_QUERYCAP, &cap, (char *)"Failed to query."); // get device capabilities
		
		if(!cap.capabilities & V4L2_CAP_STREAMING) throw v4l2_exception((char *)"This device does not support video streaming");
		
		if(!cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) throw v4l2_exception((char *)"This device does not support single planar capture");
		
		struct v4l2_input input;
		memset(&input, 0, sizeof(input));
		int index = 0;
		
		ioctl_exception(descriptor, VIDIOC_S_INPUT, &index, (char *)"Failed to set input."); // set input index, 0 is default
		
		struct v4l2_format format;
		memset(&format, 0, sizeof(format));
		
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // Set format, currently 1920x1080 H.264 encoded
		format.fmt.pix.width = CAM_WIDTH;
		format.fmt.pix.height = CAM_HEIGHT;
		format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;

		ioctl_exception(descriptor, VIDIOC_S_FMT, &format, (char *)"Failed to set format");

		struct v4l2_requestbuffers reqbuf;
		memset(&reqbuf, 0, sizeof(reqbuf));
		
		reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		reqbuf.memory = V4L2_MEMORY_MMAP;
		reqbuf.count = BUF_REQUEST_COUNT;
		
		ioctl_exception(descriptor, VIDIOC_REQBUFS, &reqbuf, (char *)"Could not request buffers");
		
		struct v4l2_streamparm param;
		memset(&param, 0, sizeof(param));
		param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
		param.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
		param.parm.capture.timeperframe.numerator = 1;
		param.parm.capture.timeperframe.denominator = 15;

		ioctl_exception(descriptor, VIDIOC_S_PARM, &param, (char *)"Could not set framerate");

		videoBuffer buffers[reqbuf.count]; // organize the buffers
		std::cout << "count: " << reqbuf.count << std::endl;
		for(int i = 0; i < reqbuf.count; i++) {
			// for each buffer, map it to userspace and queue it so that the driver can use it 
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));
			
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			
			ioctl_exception(descriptor, VIDIOC_QUERYBUF, &buf, (char *)"Failed to query buffers.");

			void* start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, buf.m.offset);
			
			if(start == MAP_FAILED) throw v4l2_exception((char *)"mmap failed to map buffers.");

			buffers[i] = {buf, start};

			memset(buffers[i].start, 0, buffers[i].bufferInfo.length);
			ioctl_exception(descriptor, VIDIOC_QBUF, &buffers[i].bufferInfo, (char *)"Failed to initially queue buffers");
		}


		int currentPos = 0;
		int sent = 0;
		int packetSize = 32;
		int thisPacket;

		AbstractSocket socket = AbstractSocket();
		socket.connect(HOST_PORT, HOST_ADDR);
		std::cout << "Connected to " << HOST_ADDR << std::endl;
		
		socket.write(&cameraInfo.cameraID, sizeof(int)); // send the camera metadata to the server
		socket.write(&cameraInfo.frameWidth, sizeof(int));
		socket.write(&cameraInfo.frameHeight, sizeof(int));
		int tmp = 0;
		int num = 0;
	
		while(1) {
			for(int i = 0; i < reqbuf.count; i++) {
				ioctl_exception(descriptor, VIDIOC_DQBUF, &buffers[i].bufferInfo, (char *)"Failed to dequeue buffer"); // dequeue buffer
				std::cout << "Len: " << buffers[i].bufferInfo.bytesused << std::endl;
				socket.write(&buffers[i].bufferInfo.bytesused, 4); // send the size of the frame to the camera
				
				while(currentPos < buffers[i].bufferInfo.bytesused) { // serialize the frame
					thisPacket = packetSize;
					if(currentPos + packetSize > buffers[i].bufferInfo.bytesused) thisPacket = buffers[i].bufferInfo.bytesused - currentPos;
					
					sent = socket.write(buffers[i].start + currentPos, thisPacket);
					currentPos += sent;
				}
				
				currentPos = 0;

				ioctl_exception(descriptor, VIDIOC_QBUF, &buffers[i].bufferInfo, (char *)"Failed to queue buffer"); // queue the buffer again

			}
		}	
	} catch(std::exception& e) { // something went wrong
		std::cout << e.what() << std::endl;
		
		int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(ioctl(descriptor, VIDIOC_STREAMOFF, &type) < 0) { // turn off streaming
			std::cerr << "Failed to disable streaming!" << std::endl;
			return -1;
		}
		
		close(descriptor); // close the descriptor
	}*/
}

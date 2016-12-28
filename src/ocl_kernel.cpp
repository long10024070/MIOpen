#include <mlopen/oclkernel.hpp>

namespace mlopen {

#ifndef NDEBUG
static std::string DimToFormattedString(const size_t* dims, size_t count)
{
	std::stringstream ss;
	ss << "{ ";
	for (size_t i = 0; i < count; ++i) {
		if (i > 0) { ss << ", "; }
		ss << dims[i];
	}
	ss << " }";
	return ss.str();
}
#endif // !NDEBUG

void OCLKernelInvoke::run() const
{
#ifndef NDEBUG
	std::cout << ", work_dim = " << work_dim;
	std::cout << ", global_work_offset = " << (work_dim == 0 ? "NULL" : DimToFormattedString(global_work_offset.data(), work_dim));
	std::cout << ", global_work_dim = " << DimToFormattedString(global_work_dim.data(), work_dim);
	std::cout << ", local_work_dim = " << (local_work_dim[0] == 0 ? "NULL" : DimToFormattedString(local_work_dim.data(), work_dim));
	std::cout << std::endl;
#endif // !NDEBUG

	cl_event ev;
	/* way to run OCL group larger than 256 
	 * hack to ensure local_size == 0, just checking that the 1st dim is 0
	 * may want to use a better solution*/
	cl_int status = clEnqueueNDRangeKernel(queue, kernel.get(),
		work_dim,
		((work_dim == 0) ? nullptr : global_work_offset.data()),
		global_work_dim.data(),
		((local_work_dim[0] == 0) ? nullptr : local_work_dim.data()),
		0, nullptr, callback ? &ev : nullptr);

	clFlush(queue);

	if (status != CL_SUCCESS) {
		MLOPEN_THROW_CL_STATUS(status, "Running kernel failed: ");
	}
	else if (callback) {
		clFinish(queue);
		clWaitForEvents(1, &ev);
		callback(ev);
	}
}

OCLKernelInvoke OCLKernel::Invoke(cl_command_queue q, std::function<void(cl_event&)> callback) const
{
#ifndef NDEBUG
	std::cout << "Info: " << "Invoking kernel: " << GetName(); // grid size + \n in OCLKernelInvoke::run()
#endif // !NDEBUG

	OCLKernelInvoke result{q, kernel, gdims.size(), {}, {}, {}, callback, compiled_ins};
	std::copy(gdims.begin(), gdims.end(), result.global_work_dim.begin());
	std::copy(ldims.begin(), ldims.end(), result.local_work_dim.begin());
	return result;
}

std::string OCLKernel::GetName() const
{
	std::array<char, 200> buffer{};

	cl_int status = clGetKernelInfo(kernel.get(), 
			CL_KERNEL_FUNCTION_NAME, 
			200, 
			buffer.data(), 
			nullptr);

	if(status != CL_SUCCESS) 
	{
		MLOPEN_THROW_CL_STATUS(status, "Error getting kernel name");
	}
	return buffer.data();
}

} // namespace mlopen

